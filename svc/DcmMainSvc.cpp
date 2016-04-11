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

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <glib.h>
#include <DcmMainSvc.h>
#include <DcmIpcUtils.h>
#include <DcmTypes.h>
#include <DcmScanSvc.h>
#include <DcmDebugUtils.h>


#define DCM_SVC_MAIN_THREAD_TIMEOUT_SEC 1

namespace DcmMainSvcCallBack {
gboolean readMsg(GIOChannel *src, GIOCondition condition, gpointer data);
gboolean quitTimerAtMainLoop(gpointer data);
}


DcmMainSvc *DcmMainSvc::dcmMainSvc = NULL;
GMainLoop *g_dcm_svc_mainloop;
static GMutex gMutexLock;

DcmMainSvc* DcmMainSvc::getInstance(void)
{
	if (dcmMainSvc == NULL) {
		g_mutex_trylock(&gMutexLock);

		if (dcmMainSvc == NULL) {
			dcmMainSvc = new DcmMainSvc();
		}

		g_mutex_unlock(&gMutexLock);
	}

	return dcmMainSvc;
}

void DcmMainSvc::dcmServiceStartjobs(void)
{
	/* Send ready response to dcm launcher */
	if (DcmIpcUtils::sendClientSocketMsg(-1, DCM_IPC_MSG_SERVICE_READY, 0, NULL, DCM_IPC_PORT_MS_RECV) != DCM_SUCCESS) {
		dcm_error("Failed to send ready message");
	}
}

void DcmMainSvc::dcmServiceFinishjobs(void)
{
	/* TODO: free resources for dcm-service */
}

int DcmMainSvc::waitScanThreadReady()
{
	int ret = DCM_SUCCESS;
	DcmIpcMsg *async_queue_msg = NULL;

	/* Wait until the scan thread is ready (timeout: 5 sec) */
	async_queue_msg = (DcmIpcMsg *)g_async_queue_timeout_pop(scan_thread_ready, 5000000);
	if (async_queue_msg == NULL) {
		dcm_error("Async queue timeout!");
		return DCM_ERROR_ASYNC_QUEUE_FAILED;
	}

	/* Check if scan thread is created */
	if (async_queue_msg->msg_type == DCM_IPC_MSG_SCAN_READY) {
		dcm_warn("DCM scan thread is ready!");
		g_async_queue_unref(scan_thread_ready);
		scan_thread_ready = NULL;
	} else {
		dcm_error("Invalid async queue message!");
		ret = DCM_ERROR_ASYNC_QUEUE_FAILED;
	}

	/* Free the received ipc message */
	DCM_SAFE_FREE(async_queue_msg);

	return ret;
}

int DcmMainSvc::createScanThread()
{
	int ret = DCM_SUCCESS;

	dcm_debug_fenter();

	/* Create a new async queue to wait util scan thread is created */
	scan_thread_ready = g_async_queue_new();
	if (scan_thread_ready == NULL) {
		dcm_error("Failed to create async queue!");
		return DCM_ERROR_ASYNC_QUEUE_FAILED;
	}

	/* Create the scan thread */
	scan_thread = g_thread_new("dcm_scan_thread", (GThreadFunc) DcmScanMain::runScanThread, (gpointer) this);
	if (scan_thread == NULL) {
		dcm_error("Failed to create scan thread!");
		return DCM_ERROR_CREATE_THREAD_FAILED;
	}

	ret = waitScanThreadReady();
	/* Wait until scan thread is ready */
	if (ret != DCM_SUCCESS) {
		dcm_error("Failed to wait for scan thread to be ready! err: %d", ret);
		return ret;
	}

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

gboolean DcmMainSvcCallBack::readMsg(GIOChannel *src, GIOCondition condition, gpointer data)
{
	DcmIpcMsg recv_msg;
	int sock = -1;
	int client_sock = -1;
	int ret = 0;

	DcmMainSvc *dcmSvc = DcmMainSvc::getInstance();

	memset((void *)&recv_msg, 0, sizeof(recv_msg));

	sock = g_io_channel_unix_get_fd(src);
	if (sock < 0) {
		dcm_error("sock fd is invalid!");
		return TRUE;
	}

	/* Accept tcp client socket */
	ret = DcmIpcUtils::acceptSocket(sock, &client_sock);
	if (ret != DCM_SUCCESS) {
		dcm_error("Failed to accept tcp socket! err: %d", ret);
		return TRUE;
	}

	if (DcmIpcUtils::receiveSocketMsg(client_sock, &recv_msg) < 0) {
		dcm_error("getRecvMsg failed");
		return TRUE;
	}

	dcm_debug("Received msg_type : [%d]", recv_msg.msg_type);

	if (dcmSvc->scan_thread == NULL) {
		dcm_debug("scan thread is not started yet!");
		/* Create scan thread before main loop is started */
		if (dcmSvc->createScanThread() != DCM_SUCCESS) {
			dcm_error("Failed to create scan thread! Exit main thread...");
			return TRUE;
		}

		dcmSvc->scan_thread_working = true;
	} else {
		dcm_debug("scan thread is already running!");
	}

	if (recv_msg.msg_type == DCM_IPC_MSG_SCAN_TERMINATED) {
		dcm_debug("Scan terminated!");
		dcmSvc->scan_thread_working = false;
		dcmSvc->createQuitTimerMainLoop();
	} else if (recv_msg.msg_type == DCM_IPC_MSG_SCAN_COMPLETED) {
		dcm_debug("Scan completed!");
		ret = DcmIpcUtils::sendClientSocketMsg(-1, DCM_IPC_MSG_SERVICE_COMPLETED, recv_msg.uid, recv_msg.msg, DCM_IPC_PORT_MS_RECV);
	} else if (recv_msg.msg_type == DCM_IPC_MSG_KILL_SERVICE) {
		dcm_warn("Quit dcm-svc main loop!");
		ret = DcmIpcUtils::sendSocketMsg(DCM_IPC_MSG_KILL_SERVICE, recv_msg.uid, recv_msg.msg, DCM_IPC_PORT_SCAN_RECV);
	} else if (recv_msg.msg_type == DCM_IPC_MSG_SCAN_ALL) {
		ret = DcmIpcUtils::sendSocketMsg(DCM_IPC_MSG_SCAN_ALL, recv_msg.uid, NULL, DCM_IPC_PORT_SCAN_RECV);
		if (ret == DCM_SUCCESS) {
			ret = DcmIpcUtils::sendClientSocketMsg(client_sock, DCM_IPC_MSG_SCAN_ALL, recv_msg.uid, NULL, DCM_IPC_PORT_DCM_RECV);
		}
	} else if (recv_msg.msg_type == DCM_IPC_MSG_SCAN_SINGLE) {
		ret = DcmIpcUtils::sendSocketMsg(DCM_IPC_MSG_SCAN_SINGLE, recv_msg.uid, recv_msg.msg, DCM_IPC_PORT_SCAN_RECV);
		if (ret == DCM_SUCCESS) {
			ret = DcmIpcUtils::sendClientSocketMsg(client_sock, DCM_IPC_MSG_SCAN_SINGLE, recv_msg.uid, recv_msg.msg, DCM_IPC_PORT_DCM_RECV);
		}
	} else {
		dcm_debug("createDcmSvcReadSocket, invalid message.");
	}

	if (DcmIpcUtils::closeSocket(client_sock) < 0) {
		dcm_stderror("close failed");
	}

	return TRUE;
}

gboolean DcmMainSvcCallBack::quitTimerAtMainLoop(gpointer data)
{
	DcmMainSvc *dcmSvcApp = (DcmMainSvc *) data;

	dcm_debug_fenter();

	DCM_CHECK_FALSE(data);

	if (dcmSvcApp->scan_thread_working == true) {
		dcm_warn("Scan thread is working! DO NOT quit main thread!");
		return TRUE;
	} else {
		dcm_warn("Quit dcm-svc main loop!");
		dcmSvcApp->quitDcmSvcMainLoop();
	}

	dcm_debug_fleave();

	return FALSE;
}

int DcmMainSvc::createQuitTimerMainLoop()
{
	GSource *quit_timer = NULL;

	dcm_debug_fenter();

	if (main_thread_quit_timer != NULL) {
		dcm_debug("Delete old quit timer!");
		g_source_destroy(main_thread_quit_timer);
		main_thread_quit_timer = NULL;
	}

	quit_timer = g_timeout_source_new_seconds(DCM_SVC_MAIN_THREAD_TIMEOUT_SEC);
	DCM_CHECK_VAL(quit_timer, DCM_ERROR_OUT_OF_MEMORY);

	g_source_set_callback(quit_timer, DcmMainSvcCallBack::quitTimerAtMainLoop, (gpointer) this, NULL);
	g_source_attach(quit_timer, main_loop_context);
	main_thread_quit_timer = quit_timer;

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

void DcmMainSvc::quitDcmSvcMainLoop()
{
	if (g_dcm_svc_mainloop != NULL) {
		dcm_debug("Quit DCM thread main loop!");
		g_main_loop_quit(g_dcm_svc_mainloop);
	} else {
		dcm_error("Invalid DCM thread main loop!");
	}
}

EXPORT_API int main(int argc, char *argv[])
{
	DcmMainSvc *dcmSvc = DcmMainSvc::getInstance();
	int sockfd = -1;

	GSource *source = NULL;
	GIOChannel *channel = NULL;
	GMainContext *context = NULL;

	/* Create and bind new socket to mainloop */
	if (DcmIpcUtils::createSocket(&sockfd, DCM_IPC_PORT_DCM_RECV) != DCM_SUCCESS) {
		dcm_error("Failed to create socket");
		return -1;
	}

	g_dcm_svc_mainloop = g_main_loop_new(context, FALSE);
	context = g_main_loop_get_context(g_dcm_svc_mainloop);

	/* Create new channel to watch new socket for mainloop */
	channel = g_io_channel_unix_new(sockfd);
	source = g_io_create_watch(channel, G_IO_IN);

	/* Set callback to be called when socket is readable */
	g_source_set_callback(source, (GSourceFunc)DcmMainSvcCallBack::readMsg, NULL, NULL);
	g_source_attach(source, context);

	dcmSvc->main_loop_context = context;

	dcmSvc->dcmServiceStartjobs();

	dcm_debug("********************************************");
	dcm_debug("****** DCM Service is running ******");
	dcm_debug("********************************************");

	g_main_loop_run(g_dcm_svc_mainloop);

	dcmSvc->dcmServiceFinishjobs();

	dcm_debug("DCM Service is shutting down...");
	g_io_channel_shutdown(channel, FALSE, NULL);
	g_io_channel_unref(channel);
	close(sockfd);
	g_main_loop_unref(g_dcm_svc_mainloop);

	return 0;
}
