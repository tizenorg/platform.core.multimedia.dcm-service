/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <vconf.h>
#include <img-codec-parser.h>

#include "../libdcm-util/include/dcm_image_codec.h"
#include <DcmDbUtils.h>
#include <DcmColorUtils.h>
#include <DcmFaceUtils.h>
#include <DcmIpcUtils.h>
#include <DcmTypes.h>
#include <DcmScanSvc.h>
#include <DcmMainSvc.h>
#include <DcmDebugUtils.h>
#include <Ecore_Evas.h>

#define MIME_TYPE_JPEG "image/jpeg"
#define MIME_TYPE_PNG "image/png"
#define MIME_TYPE_BMP "image/bmp"

class DcmScanSvc {
public:
	GMainLoop *g_scan_thread_mainloop;
	GMainContext *scan_thread_main_context;

	/* scan all images */
	GList *scan_all_item_list;
	unsigned int scan_all_curr_index;

	/* scan single images */
	GList *scan_single_item_list;
	unsigned int scan_single_curr_index;

	void quitScanThread();
	int getMmcState(void);
	int prepareImageList();
	int prepareImageListByPath(const char *file_path);
	int clearAllItemList();
	int clearSingleItemList();
	int initialize();
	int finalize();
	int sendCompletedMsg(const char *msg, DcmIpcPortType port);
	int getScanStatus(DcmScanItem *scan_item, bool *media_scanned);
	int runScanProcess(DcmScanItem *scan_item);
	int ScanAllItems();
	int ScanSingleItem(const char *file_path);
	int terminateScanOperations();
	int receiveMsg(DcmIpcMsg *recv_msg);

};

namespace DcmScanCallback {
	void freeScanItem(void *data);
	gboolean readyScanThreadIdle(gpointer data);
	gboolean readMsg(GIOChannel *src, GIOCondition condition, gpointer data);
}

void DcmScanSvc::quitScanThread()
{
	if (g_scan_thread_mainloop != NULL) {
		dcm_warn("Quit scan thread mainloop!");
		g_main_loop_quit(g_scan_thread_mainloop);
	} else {
		dcm_warn("Scan thread mainloop is invalid!");
	}

	return;
}

int DcmScanSvc::getMmcState(void)
{
	int err = -1;
	int status = -1;

	err = vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &status);
	if (err != 0) {
		dcm_error("vconf_get_int Unexpected error code: %d", err);
	}

	return status;
}

int DcmScanSvc::prepareImageList()
{
	int ret = DCM_SUCCESS;
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();
	bool mmc_mounted = false;

	if (getMmcState() == VCONFKEY_SYSMAN_MMC_MOUNTED) {
		mmc_mounted = true;
	} else {
		mmc_mounted = false;
	}

	/* Get scan image list from db */
	ret = dcmDbUtils->_dcm_svc_db_get_scan_image_list_from_db(&(scan_all_item_list), mmc_mounted);
	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("Failed to get image list from db! ret: %d", ret);
		return ret;
	}

	if (scan_all_item_list == NULL) {
		dcm_debug("No image list for scanning");
		return DCM_ERROR_DB_NO_RESULT;
	}

	if ((scan_all_item_list != NULL) && (g_list_length(scan_all_item_list) == 0)) {
		dcm_debug("No image list from db!");
		return DCM_ERROR_DB_NO_RESULT;
	}

	return DCM_SUCCESS;
}

int DcmScanSvc::prepareImageListByPath(const char *file_path)
{
	int ret = DCM_SUCCESS;
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();
	bool mmc_mounted = false;

	if (getMmcState() == VCONFKEY_SYSMAN_MMC_MOUNTED) {
		mmc_mounted = true;
	} else {
		mmc_mounted = false;
	}

	/* Get scan image list from db */
	ret = dcmDbUtils->_dcm_svc_db_get_scan_image_list_by_path(&(scan_single_item_list), mmc_mounted, file_path);
	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("Failed to get image list from db! ret: %d", ret);
		return ret;
	}

	if (scan_single_item_list == NULL) {
		dcm_debug("No image list for scanning");
		return DCM_ERROR_DB_NO_RESULT;
	}

	if ((scan_single_item_list != NULL) && (g_list_length(scan_single_item_list) == 0)) {
		dcm_debug("No image list from db!");
		return DCM_ERROR_DB_NO_RESULT;
	}

	return DCM_SUCCESS;
}

int DcmScanSvc::clearAllItemList()
{
	dcm_debug_fenter();

	if (scan_all_item_list != NULL) {
		g_list_free_full(scan_all_item_list, DcmScanCallback::freeScanItem);
		scan_all_item_list = NULL;
	}

	scan_all_curr_index = 0;

	return DCM_SUCCESS;
}

int DcmScanSvc::clearSingleItemList()
{
	dcm_debug_fenter();

	if (scan_single_item_list) {
		g_list_free_full(scan_single_item_list, DcmScanCallback::freeScanItem);
		scan_single_item_list = NULL;
	}

	scan_single_curr_index = 0;

	return DCM_SUCCESS;
}

int DcmScanSvc::initialize()
{
	scan_all_item_list = NULL;
	scan_all_curr_index = 0;
	scan_single_item_list = NULL;
	scan_single_curr_index = 0;

	DcmFaceUtils::initialize();

	ecore_evas_init();

	return DCM_SUCCESS;
}

int DcmScanSvc::finalize()
{
	/* Only scan item lists are freed here, scan idles should be freed before this function */
	clearAllItemList();
	clearSingleItemList();
	DcmFaceUtils::finalize();

	ecore_evas_shutdown();

	return DCM_SUCCESS;
}

int DcmScanSvc::sendCompletedMsg(const char *msg, DcmIpcPortType port)
{
	if ((scan_all_item_list == NULL) && (scan_single_item_list == NULL)) {
		dcm_debug("Send completed message");
		DcmIpcUtils::sendSocketMsg(DCM_IPC_MSG_SCAN_COMPLETED, 0, msg, port);
	} else {
		if (scan_all_item_list)
			dcm_warn("scan_all_item_list");

		if (scan_single_item_list)
			dcm_warn("scan_single_item_list");

		dcm_warn("Scan operation is not finished yet. Keep scanning...");
	}

	return DCM_SUCCESS;
}

int DcmScanSvc::getScanStatus(DcmScanItem *scan_item, bool *media_scanned)
{
	int ret = DCM_SUCCESS;
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();

	DCM_CHECK_VAL(scan_item, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(scan_item->media_uuid, DCM_ERROR_INVALID_PARAMETER);

	/* Check if this media is scanned or not */
	ret = dcmDbUtils->_dcm_svc_db_check_scanned_by_media_uuid(scan_item->media_uuid, media_scanned);
	if (ret != DCM_SUCCESS) {
		dcm_error("Failed to check if this media item is scanned or not!");
	}

	return ret;
}

int DcmScanSvc::runScanProcess(DcmScanItem *scan_item)
{
	bool media_scanned = false;
	int ret = DCM_SUCCESS;

	dcm_debug_fenter();
	DCM_CHECK_VAL(scan_item, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(scan_item->file_path, DCM_ERROR_INVALID_PARAMETER);

	DcmImageInfo image_info = {0, };
	memset(&image_info, 0, sizeof(DcmImageInfo));
	dcm_image_codec_type_e image_format = DCM_IMAGE_CODEC_RGB888;

	/* Process scan operation if the file exists */
	if (g_file_test(scan_item->file_path, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) == TRUE) {
		/* Get necessary information from db again.
		* Media information will be inserted after face is detected.
		* If media uuid does not exist, retry is needed */
		ret = getScanStatus(scan_item, &media_scanned);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to get scan item info from db! err: %d", ret);
			return ret;
		}

		/* It is possible that when single and async scan for the same image is in the list.
		* If the media uuid is already scanned, skip this scan. */
		if (media_scanned == true) {
			dcm_warn("This media is scanned already! Skip...");
			DCM_SAFE_FREE(image_info.pixel);
			return DCM_ERROR_IMAGE_ALREADY_SCANNED;
		} else {
			dcm_debug("This media is NOT scanned yet.");
		}

		dcm_sec_debug("scan file path : [%s]", scan_item->file_path);
		dcm_sec_debug("scan media uuid : [%s]", scan_item->media_uuid);

		ImgCodecType type = IMG_CODEC_NONE;

		image_info.original_width = scan_item->image_width;
		image_info.original_height = scan_item->image_height;

		dcm_debug("scan media w : [%d], h : [%d], orientation : [%d]", image_info.original_width, image_info.original_height, scan_item->image_orientation);

		if (image_info.original_width <= 0 && image_info.original_height <= 0) {
			ret = ImgGetImageInfo((const char *)(scan_item->file_path), &type, &(image_info.original_width), &(image_info.original_height));
			if (ret != DCM_SUCCESS) {
				dcm_error("Failed ImgGetImageInfo! err: %d", ret);
				return ret;
			}

			dcm_debug("ImgGetImageInfo type: %d, width: %d, height: %d", type, image_info.original_width, image_info.original_height);
		}

		if (strcmp(scan_item->mime_type, MIME_TYPE_JPEG) == 0) {
			ret = dcm_decode_image_with_size_orient((const char *) (scan_item->file_path), image_info.original_width,
				image_info.original_height, image_format, &(image_info.pixel), &(image_info.buffer_width), &(image_info.buffer_height), &(image_info.orientation), &(image_info.size));
			if (ret != DCM_SUCCESS) {
				dcm_error("Failed dcm_decode_image_with_size_orient! err: %d", ret);
				return ret;
			}
		} else if ((strcmp(scan_item->mime_type, MIME_TYPE_PNG) == 0) || (strcmp(scan_item->mime_type, MIME_TYPE_BMP) == 0)) {
			ret = dcm_decode_image_with_evas((const char *) (scan_item->file_path), image_info.original_width,
				image_info.original_height, image_format, &(image_info.pixel), &(image_info.buffer_width), &(image_info.buffer_height), &(image_info.orientation), &(image_info.size));
			if (ret != DCM_SUCCESS) {
				dcm_error("Failed dcm_decode_image_with_evas! err: %d", ret);
				return ret;
			}
		} else {
			dcm_error("Failed not supported type! (%s)", scan_item->mime_type);
			return DCM_ERROR_INVALID_PARAMETER;
		}
		image_info.decode_type = (DcmImageDecodeType)image_format;

		dcm_debug("Image info width: %d, height: %d, buf_width: %d, buf_height: %d, orientation: %d",
			image_info.original_width, image_info.original_height, image_info.buffer_width, image_info.buffer_height, image_info.orientation);

		/* Process face scan */
		ret = DcmFaceUtils::runFaceRecognizeProcess(scan_item, &image_info);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to process face detection! err: %d", ret);
		}

		/* Set sleep time after face recognition */
		usleep(500000);

		/* Process color extract */
		ret = DcmColorUtils::runColorExtractProcess(scan_item, &image_info);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to process color extraction! err: %d", ret);
		}

		/* Free image buffer */
		DCM_SAFE_FREE(image_info.pixel);
	} else {
		dcm_warn("The file does not exist! Skip dcm scan for this file ...");
	}

	dcm_debug_fleave();

	return ret;
}

int DcmScanSvc::ScanAllItems()
{
	int ret = DCM_SUCCESS;
	DcmScanItem *scan_item = NULL;
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();

	dcm_debug_fenter();

	clearAllItemList();

	ret = prepareImageList();
	if (ret == DCM_ERROR_DB_NO_RESULT) {
		dcm_debug("No items to Scan. Scan operation completed!!!");
		clearAllItemList();
		/* Send scan complete message to main thread (if all scan operations are finished) */
		sendCompletedMsg( NULL, DCM_IPC_PORT_DCM_RECV);
		return DCM_SUCCESS;
	}

	/* DCM scan started */
	unsigned int list_len = g_list_length(scan_all_item_list);
	while (scan_all_curr_index < list_len) {
		scan_item = (DcmScanItem *)g_list_nth_data(scan_all_item_list, scan_all_curr_index);
		dcm_sec_debug("current index: %d, path: %s", scan_all_curr_index, scan_item->file_path);

		ret = runScanProcess(scan_item);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to process scan job! err: %d", ret);

			/* If the scan item is not scanned, insert media uuid into face_scan_list */
			if (ret != DCM_ERROR_IMAGE_ALREADY_SCANNED) {
				dcmDbUtils->_dcm_svc_db_insert_face_to_face_scan_list(scan_item);
			}
		}

		(scan_all_curr_index)++;
	}

	dcm_warn("All images are scanned. Scan operation completed");

	clearAllItemList();
	/* Send scan complete message to main thread (if all scan operations are finished) */
	sendCompletedMsg( NULL, DCM_IPC_PORT_DCM_RECV);

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

int DcmScanSvc::ScanSingleItem(const char *file_path)
{
	int ret = DCM_SUCCESS;
	DcmScanItem *scan_item = NULL;
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();

	DCM_CHECK_VAL(file_path, DCM_ERROR_INVALID_PARAMETER);

	dcm_debug_fenter();

	clearSingleItemList();

	ret = prepareImageListByPath(file_path);
	if (ret == DCM_ERROR_DB_NO_RESULT) {
		dcm_debug("No items to Scan. Scan operation completed!!!");
		clearSingleItemList();
		/* Send scan complete message to main thread (if all scan operations are finished) */
		sendCompletedMsg( file_path/*ret*/, DCM_IPC_PORT_DCM_RECV);
		return DCM_SUCCESS;
	}

	dcm_debug("append scan item to scan item list");

	/* DCM scan started */
	unsigned int list_len = g_list_length(scan_single_item_list);
	if (scan_single_curr_index < list_len) {
		scan_item = (DcmScanItem *)g_list_nth_data(scan_single_item_list, scan_single_curr_index);
		dcm_sec_debug("current index: %d, path: %s, scan type: %d", scan_single_curr_index, scan_item->file_path, scan_item->scan_item_type);

		ret = runScanProcess(scan_item);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to process scan job! err: %d", ret);

			/* If the scan item is not scanned, insert media uuid into face_scan_list */
			if (ret != DCM_ERROR_IMAGE_ALREADY_SCANNED) {
				dcmDbUtils->_dcm_svc_db_insert_face_to_face_scan_list(scan_item);
			}
		}

		(scan_single_curr_index)++;
	}

	clearSingleItemList();

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

int DcmScanSvc::terminateScanOperations()
{
	dcm_debug("Terminate scanning operations, and quit scan thread main loop");

	quitScanThread();

	return DCM_SUCCESS;
}

int DcmScanSvc::receiveMsg(DcmIpcMsg *recv_msg)
{
	int ret = DCM_SUCCESS;

	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();
	DCM_CHECK_VAL(recv_msg, DCM_ERROR_INVALID_PARAMETER);

	ret = dcmDbUtils->_dcm_svc_db_connect(recv_msg->uid);
	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("Failed to connect DB! err: %d", ret);
		return DCM_ERROR_DB_OPERATION;
	}

	if (recv_msg->msg_type == DCM_IPC_MSG_SCAN_ALL)
	{
		/* Use timer to scan all unscanned images */
		ScanAllItems();
	}
	else if (recv_msg->msg_type == DCM_IPC_MSG_SCAN_SINGLE)
	{
		/* Create a scan idle if not exist, and scan single image of which file path is reveived from tcp socket */
		if (recv_msg->msg_size > 0 && recv_msg->msg_size < DCM_IPC_MSG_MAX_SIZE) {
			char *file_path = NULL;
			file_path = strdup(recv_msg->msg);
			if (file_path != NULL) {
				ScanSingleItem((const char *) file_path);
				DCM_SAFE_FREE(file_path);
			} else {
				dcm_error("Failed to copy message!");
				ret = DCM_ERROR_OUT_OF_MEMORY;
			}
		} else {
			dcm_error("Invalid receive message!");
			ret = DCM_ERROR_IPC_INVALID_MSG;
		}
	}
	else if (recv_msg->msg_type == DCM_IPC_MSG_SCAN_TERMINATED)
	{
		/* Destroy scan idles, and quit scan thread main loop */
		terminateScanOperations();
	}
	else {
		dcm_error("Invalid message type!");
		ret = DCM_ERROR_IPC_INVALID_MSG;
	}

	ret = dcmDbUtils->_dcm_svc_db_disconnect();
	if (ret != DCM_SUCCESS) {
		dcm_error("Failed to disconnect DB! err: %d", ret);
		return DCM_ERROR_DB_OPERATION;
	}

	return ret;
}

gboolean DcmScanCallback::readMsg(GIOChannel *src, GIOCondition condition, gpointer data)
{
	DcmScanSvc *dcmScanSvc = (DcmScanSvc *) data;
	int sock = -1;
	int client_sock = -1;
	DcmIpcMsg recv_msg;
	int err = 0;

	DCM_CHECK_FALSE(data);

	/* Get socket fd from IO channel */
	sock = g_io_channel_unix_get_fd(src);
	if (sock < 0) {
		dcm_error("Invalid socket fd!");
		return TRUE;
	}

	/* Accept tcp client socket */
	err = DcmIpcUtils::acceptSocket(sock, &client_sock);
	if (err != DCM_SUCCESS) {
		dcm_error("Failed to accept tcp socket! err: %d", err);
		return TRUE;
	}

	/* Receive message from tcp socket */
	err = DcmIpcUtils::receiveSocketMsg(client_sock, &recv_msg);
	if (err != DCM_SUCCESS) {
		dcm_error("Failed to receive tcp msg! err: %d", err);
		goto DCM_SVC_SCAN_READ_THREAD_RECV_SOCKET_FAILED;
	}

	/* Process received message */
	err = dcmScanSvc->receiveMsg(&recv_msg);
	if (err != DCM_SUCCESS) {
		dcm_error("Error ocurred when process received message: %d", err);
		goto DCM_SVC_SCAN_READ_THREAD_RECV_SOCKET_FAILED;
	}

DCM_SVC_SCAN_READ_THREAD_RECV_SOCKET_FAILED:

	if (close(client_sock) < 0) {
		dcm_error("close failed [%s]", strerror(errno));
	}

	return TRUE;
}

gboolean DcmScanCallback::readyScanThreadIdle(gpointer data)
{
	DcmMainSvc *ad = (DcmMainSvc *) data;
	DcmIpcMsg *async_queue_msg = NULL;

	dcm_debug_fenter();

	DCM_CHECK_FALSE(data);
	DCM_CHECK_FALSE(ad->scan_thread_ready);

	async_queue_msg = (DcmIpcMsg*) g_malloc0(sizeof(DcmIpcMsg));
	async_queue_msg->msg_type = DCM_IPC_MSG_SCAN_READY;

	dcm_debug("scan thread ready : %p", ad->scan_thread_ready);
	dcm_debug("async_queue_msg : %d", async_queue_msg->msg_type);

	g_async_queue_push(ad->scan_thread_ready, (gpointer) async_queue_msg);

	dcm_debug_fleave();

	return FALSE;
}

void DcmScanCallback::freeScanItem(void *data)
{
	DcmScanItem *scan_item = (DcmScanItem *) data;

	dcm_debug_fenter();

	DCM_CHECK(scan_item);
	DCM_CHECK(scan_item->file_path);
	DCM_CHECK(scan_item->media_uuid);
	DCM_CHECK(scan_item->storage_uuid);

	dcm_sec_debug("Free scan item. path: [%s]", scan_item->file_path);

	DCM_SAFE_FREE(scan_item->file_path);
	DCM_SAFE_FREE(scan_item->media_uuid);
	DCM_SAFE_FREE(scan_item->storage_uuid);
	DCM_SAFE_FREE(scan_item);

	dcm_debug_fleave();

	return;
}

gboolean DcmScanMain::runScanThread(void *data)
{
	DcmMainSvc *dcmManSvc = (DcmMainSvc*) data;
	DcmScanSvc dcmScanSvc;
	int socket_fd = -1;
	GSource *source = NULL;
	GIOChannel *channel = NULL;
	GMainContext *context = NULL;
	GSource *scan_thread_ready_idle = NULL;
	int err = 0;

	DCM_CHECK_VAL(data, DCM_ERROR_INVALID_PARAMETER);

	/* Init global variables */
	err = dcmScanSvc.initialize();
	if (err != DCM_SUCCESS) {
		dcm_error("Failed to initialize scan thread global variable! err: %d", err);
		goto DCM_SVC_SCAN_CREATE_SCAN_THREAD_FAILED;
	}

	/* Create TCP Socket to receive message from main thread */
	err = DcmIpcUtils::createSocket(&socket_fd, DCM_IPC_PORT_SCAN_RECV);
	if (err != DCM_SUCCESS) {
		dcm_error("Failed to create socket! err: %d", err);
		goto DCM_SVC_SCAN_CREATE_SCAN_THREAD_FAILED;
	}
	dcm_sec_warn("scan thread recv socket: %d", socket_fd);

	/* Create a new main context for scan thread */
	context = g_main_context_new();
	dcmScanSvc.scan_thread_main_context = context;

	/* Create a new main event loop */
	dcmScanSvc.g_scan_thread_mainloop = g_main_loop_new(context, FALSE);

	/* Create a new channel to watch TCP socket */
	channel = g_io_channel_unix_new(socket_fd);
	source = g_io_create_watch(channel, G_IO_IN);

	/* Attach channel to main context in scan thread */
	g_source_set_callback(source, (GSourceFunc) DcmScanCallback::readMsg, (gpointer) &dcmScanSvc, NULL);
	g_source_attach(source, context);

	/* Create a idle after scan thread is ready */
	scan_thread_ready_idle = g_idle_source_new();
	g_source_set_callback(scan_thread_ready_idle, DcmScanCallback::readyScanThreadIdle, (gpointer) dcmManSvc, NULL);
	g_source_attach(scan_thread_ready_idle, context);

	/* Push main context to scan thread */
	g_main_context_push_thread_default(context);

	dcm_debug("********************************************");
	dcm_debug("*** DCM Service scan thread is running ***");
	dcm_debug("********************************************");

	/* Start to run main event loop for scan thread */
	g_main_loop_run(dcmScanSvc.g_scan_thread_mainloop);

	dcm_debug("*** DCM Service scan thread will be closed ***");

	/* Destroy IO channel */
	g_io_channel_shutdown(channel, FALSE, NULL);
	g_io_channel_unref(channel);

	/* Close the TCP socket */
	close(socket_fd);

	/* Descrease the reference count of main loop of scan thread */
	g_main_loop_unref(dcmScanSvc.g_scan_thread_mainloop);
	dcmScanSvc.g_scan_thread_mainloop = NULL;

DCM_SVC_SCAN_CREATE_SCAN_THREAD_FAILED:

	err = dcmScanSvc.finalize();
	if (err != DCM_SUCCESS) {
		dcm_error("Failed to de-initialize scan thread global variable! err: %d", err);
	}

	return FALSE;
}

