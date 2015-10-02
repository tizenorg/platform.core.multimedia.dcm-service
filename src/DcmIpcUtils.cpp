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
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <DcmIpcUtils.h>
#include <DcmTypes.h>
#include <DcmDebugUtils.h>

static char DCM_IPC_PATH[5][100] =
			{"/var/run/media-server/dcm_ipc_scanthread_recv",
 			 "/var/run/media-server/dcm_ipc_thumbserver_comm_recv",
 			 "/var/run/media-server/media_ipc_thumbdcm_dcmrecv"};

int DcmIpcUtils::receiveSocketMsg(int client_sock, DcmIpcMsg *recv_msg)
{
	int recv_msg_size = 0;

	if ((recv_msg_size = read(client_sock, recv_msg, sizeof(DcmIpcMsg))) < 0) {
		if (errno == EWOULDBLOCK) {
			dcm_error("Timeout. Can't try any more");
			return DCM_ERROR_IPC;
		} else {
			dcm_error("recv failed : %s", strerror(errno));
			return DCM_ERROR_NETWORK;
		}
	}
	dcm_sec_debug("[receive msg] type: %d, uid: %d, msg: %s, msg_size: %d", recv_msg->msg_type, recv_msg->uid, recv_msg->msg, recv_msg->msg_size);

	if (!(recv_msg->msg_type >= 0 && recv_msg->msg_type < DCM_IPC_MSG_MAX)) {
		dcm_error("IPC message is wrong!");
		return DCM_ERROR_IPC_INVALID_MSG;
	}

	return DCM_SUCCESS;
}

int DcmIpcUtils::acceptSocket(int serv_sock, int* client_sock)
{
	DCM_CHECK_VAL(client_sock, DCM_ERROR_INVALID_PARAMETER);
	int sockfd = -1;
	struct sockaddr_un client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	if ((sockfd = accept(serv_sock, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
		dcm_error("accept failed : %s", strerror(errno));
		*client_sock = -1;
		return DCM_ERROR_NETWORK;
	}

	*client_sock = sockfd;

	return DCM_SUCCESS;
}

int DcmIpcUtils::createSocket(int *socket_fd, DcmIpcPortType port)
{
	DCM_CHECK_VAL(socket_fd, DCM_ERROR_INVALID_PARAMETER);
	int sock = -1;
	struct sockaddr_un serv_addr;
	bool bind_success = false;
	int i = 0;

	/* Create a new TCP socket */
	if ((sock = socket(PF_FILE, SOCK_STREAM, 0)) < 0) {
		dcm_error("socket failed: %s", strerror(errno));
		return DCM_ERROR_NETWORK;
	}

	/* Set socket address */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	unlink(DCM_IPC_PATH[port]);
	strcpy(serv_addr.sun_path, DCM_IPC_PATH[port]);

	/* Bind socket to local address */
	for (i = 0; i < 20; i++) {
		if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
			bind_success = true;
			break;
		}
		dcm_debug("#%d bind", i);
		usleep(250000);
	}

	if (bind_success == false) {
		dcm_error("bind failed : %s %d_", strerror(errno), errno);
		close(sock);
		return DCM_ERROR_NETWORK;
	}
	dcm_debug("bind success");

	/* Listenint */
	if (listen(sock, SOMAXCONN) < 0) {
		dcm_error("listen failed : %s", strerror(errno));
		close(sock);
		return DCM_ERROR_NETWORK;
	}
	dcm_debug("Listening...");

	/* change permission of local socket file */
	if (chmod(DCM_IPC_PATH[port], 0660) < 0)
		dcm_error("chmod failed [%s]", strerror(errno));
	/*
	if (chown(DCM_IPC_PATH[port], 0, 5000) < 0)
		dcm_dbgE("chown failed [%s]", strerror(errno));
	*/

	*socket_fd = sock;

	return DCM_SUCCESS;
}

int DcmIpcUtils::sendSocketMsg(DcmIpcMsgType msg_type, uid_t uid, const char *msg, DcmIpcPortType port)
{
	if (port < 0 || port >= DCM_IPC_PORT_MAX) {
		dcm_error("Invalid port! Stop sending message...");
		return DCM_ERROR_INVALID_PARAMETER;
	}
	dcm_debug("Send message type: %d", msg_type);

	int socket_fd = -1;
	struct sockaddr_un serv_addr;
	//struct timeval tv_timeout = { 10, 0 }; /* timeout: 10 seconds */
	DcmIpcMsg send_msg;

	/* Prepare send message */
	memset((void *)&send_msg, 0, sizeof(DcmIpcMsg));
	send_msg.msg_type = msg_type;
	send_msg.uid = uid;
	if (msg != NULL) {
		send_msg.msg_size = strlen(msg);
		strncpy(send_msg.msg, msg, send_msg.msg_size);
	}

	/* If message size is larget than max_size, then message is invalid */
	if (send_msg.msg_size >= DCM_IPC_MSG_MAX_SIZE) {
		dcm_error("Message size is invalid!");
		return DCM_ERROR_INVALID_IMAGE_SIZE;
	}

	/* Create a new TCP socket */
	if ((socket_fd = socket(PF_FILE, SOCK_STREAM, 0)) < 0) {
		dcm_error("socket failed: %s", strerror(errno));
		return DCM_ERROR_NETWORK;
	}

	/* Set dcm thread socket address */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, DCM_IPC_PATH[port]);

	/* Connect to the socket */
	if (connect(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		dcm_error("connect error : %s", strerror(errno));
		close(socket_fd);
		return DCM_ERROR_NETWORK;
	}

	/* Send msg to the socket */
	if (send(socket_fd, &send_msg, sizeof(send_msg), 0) != sizeof(send_msg)) {
		dcm_error("send failed : %s", strerror(errno));
		close(socket_fd);
		return DCM_ERROR_NETWORK;
	}

	close(socket_fd);
	return DCM_SUCCESS;
}

int DcmIpcUtils::closeSocket(int socket_fd)
{
	close(socket_fd);
	return DCM_SUCCESS;
}

#define DCM_SVC_FACE_ASYNC_MEDIA_UUID_LENGTH 64
#define DCM_SVC_FACE_ASYNC_UDP_SEND_PORT 1551
#define DCM_SVC_FACE_ASYNC_INET_ADDR "127.0.0.1"
typedef struct
{
	int type; /* type should be set to 0 */
	char media_uuid[DCM_SVC_FACE_ASYNC_MEDIA_UUID_LENGTH];
} dcm_svc_face_async_msg_s;

int DcmIpcUtils::sendFaceMsg(const char *media_uuid)
{
	dcm_debug_fenter();
	DCM_CHECK_VAL(media_uuid, DCM_ERROR_INVALID_PARAMETER);
	dcm_sec_debug("media_id: %s", media_uuid);
	struct sockaddr_in client_addr;
	int socket_fd = 0;
	int ret = 0;

	/* Create send socket */
	if ((socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		dcm_error("Failed to create socket for libface-svc! err: %s", strerror(errno));
		return DCM_ERROR_NETWORK;
	}
	dcm_debug("socket fd: %d", socket_fd);

	/* Set send message */
	dcm_svc_face_async_msg_s face_async_msg;
	memset(&face_async_msg, 0x00, sizeof(dcm_svc_face_async_msg_s));
	face_async_msg.type = 0;
	g_strlcpy(face_async_msg.media_uuid, media_uuid, sizeof(face_async_msg.media_uuid));

	/* Set face async port and address */
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(DCM_SVC_FACE_ASYNC_UDP_SEND_PORT);
	client_addr.sin_addr.s_addr = inet_addr(DCM_SVC_FACE_ASYNC_INET_ADDR);

	/* Send message to libface-svc */
	ret = sendto(socket_fd, &face_async_msg, sizeof(dcm_svc_face_async_msg_s), MSG_NOSIGNAL, (struct sockaddr*)&client_addr, sizeof(client_addr));
	if (ret < 0) {
		dcm_error("Failed to send message to libface-svc! err: %s", strerror(errno));
		close(socket_fd);
		return DCM_ERROR_NETWORK;
	}

	close(socket_fd);
	dcm_debug_fleave();
	return DCM_SUCCESS;
}

