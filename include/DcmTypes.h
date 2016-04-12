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

#ifndef _DCM_TYPES_H_
#define _DCM_TYPES_H_

#include <glib.h>
#include <unistd.h>

/*
 * Definitions defined here can be used by all threads
 * If a definition can only be used in a specific thread, then this definition should be defined in that thread, not here.
 */

#define DCM_MAX_PATH_SIZE 4096
#define DCM_IPC_MSG_MAX_SIZE 4096
#define DCM_DB_QUERY_MAX_SIZE 4096

typedef enum {
	DCM_SUCCESS,
	DCM_ERROR_INVALID_PARAMETER,
	DCM_ERROR_NETWORK,
	DCM_ERROR_DB_OPERATION,
	DCM_ERROR_DB_NO_RESULT,
	DCM_ERROR_FILE_NOT_EXIST,
	DCM_ERROR_OUT_OF_MEMORY,
	DCM_ERROR_IMAGE_ALREADY_SCANNED,
	DCM_ERROR_CREATE_THREAD_FAILED,
	DCM_ERROR_IMAGE_UTIL_FAILED,
	DCM_ERROR_ASYNC_QUEUE_FAILED,
	DCM_ERROR_IPC,
	DCM_ERROR_IPC_INVALID_MSG,
	DCM_ERROR_INVALID_IMAGE_SIZE,
	DCM_ERROR_CODEC_DECODE_FAILED,
	DCM_ERROR_CODEC_ENCODE_FAILED,
	DCM_ERROR_UNSUPPORTED_IMAGE_TYPE,
	DCM_ERROR_FACE_ENGINE_FAILED,
	DCM_ERROR_EXIF_FAILED,
	DCM_ERROR_UUID_GENERATE_FAILED,
	DCM_ERROR_DUPLICATED_DATA,
} DcmErrorType;

typedef enum {
	DCM_IPC_MSG_SCAN_SINGLE,
	DCM_IPC_MSG_SCAN_ALL,
	DCM_IPC_MSG_CANCEL,
	DCM_IPC_MSG_CANCEL_ALL,
	DCM_IPC_MSG_KILL_SERVICE,
	DCM_IPC_MSG_SCAN_READY,
	DCM_IPC_MSG_SCAN_COMPLETED,
	DCM_IPC_MSG_SCAN_TERMINATED,
	DCM_IPC_MSG_SERVICE_READY = 20,
	DCM_IPC_MSG_SERVICE_COMPLETED,
	DCM_IPC_MSG_MAX,
} DcmIpcMsgType;

typedef enum {
	DCM_IPC_PORT_SCAN_RECV = 0,
	DCM_IPC_PORT_DCM_RECV,
	DCM_IPC_PORT_MS_RECV,
	DCM_IPC_PORT_MAX,
} DcmIpcPortType;

typedef struct {
	DcmIpcMsgType msg_type;
	int pid;
	uid_t uid;
	size_t msg_size; /*this is size of message below and this does not include the terminationg null byte ('\0'). */
	char msg[DCM_IPC_MSG_MAX_SIZE];
} DcmIpcMsg;

typedef enum {
	DCM_SCAN_ITEM_TYPE_NONE,
	DCM_SCAN_ITEM_TYPE_SCAN_ALL,
	DCM_SCAN_ITEM_TYPE_SCAN_SINGLE,
	DCM_SCAN_ITEM_TYPE_MAX,
} DcmScanItemType;

typedef struct {
	char *media_uuid;
	char *file_path;
	char *storage_uuid;
	int image_width;
	int image_height;
	int image_orientation;
	char *mime_type;
	DcmScanItemType scan_item_type;
} DcmScanItem;

typedef enum {
	DCM_FACE_ITEM_UPDATE_FACE	= 0,	/**< One face item is upated */
} DcmFaceItemUpdateItem;

typedef enum {
	DCM_FACE_ITEM_INSERT		= 0,	/**< Database update operation is INSERT */
	DCM_FACE_ITEM_DELETE		= 1,	/**< Database update operation is DELETE */
	DCM_FACE_ITEM_UPDATE		= 2,	/**< Database update operation is UPDATE */
	DCM_FACE_ITEM_REMOVE		= 3,	/**< Database update operation is REMOVE */
} DcmFaceItemUpdateType;

typedef struct {
	char *face_uuid;
	char *media_uuid;
	unsigned int face_rect_x;
	unsigned int face_rect_y;
	unsigned int face_rect_w;
	unsigned int face_rect_h;
	int orientation;
} DcmFaceItem;

typedef struct {
	char *media_uuid;
	char *storage_uuid;
	unsigned char rgb_r;
	unsigned char rgb_g;
	unsigned char rgb_b;
} DcmColorItem;

typedef enum {
	DCM_SVC_I420,
	DCM_SVC_RGB,
	DCM_SVC_RGBA,
} DcmImageDecodeType;

typedef struct {
	unsigned char *pixel;		/* decoding results, must be freed after use */
	unsigned long long size;
	int orientation;			/* orientation information extracted from exif */
	unsigned int original_width;			/* original image width */
	unsigned int original_height;		/* original image height */
	unsigned int buffer_width;			/* scaled image width used by decoder (width/height ratio should be the same as original) */
	unsigned int buffer_height;			/* scaled image height used by decoder (width/height ratio should be the same as original) */
	DcmImageDecodeType decode_type;	/* decoding pre-condition */
} DcmImageInfo;

#endif /* _DCM_TYPES_H_ */

