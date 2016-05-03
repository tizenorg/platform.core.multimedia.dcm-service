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

static char DCM_IPC_PATH[][100] = {
			{"/tmp/media-server/dcm_ipc_scanthread.socket"},
			{"/tmp/media-server/media_ipc_dcmdaemon.socket"},
			{"/tmp/media-server/media_ipc_dcmcomm.socket"},
};

int DcmIpcUtils::receiveSocketMsg(int client_sock, DcmIpcMsg *recv_msg)
{
	int recv_msg_size = 0;

	if ((recv_msg_size = read(client_sock, recv_msg, sizeof(DcmIpcMsg))) < 0) {
		if (errno == EWOULDBLOCK) {
			dcm_error("Timeout. Can't try any more");
			return DCM_ERROR_IPC;
		} else {
			dcm_stderror("recv failed");
			return DCM_ERROR_NETWORK;
		}
	}
	dcm_sec_debug("[receive msg] type: %d, pid: %d, uid: %d, msg: %s, msg_size: %d", recv_msg->msg_type, recv_msg->pid, recv_msg->uid, (recv_msg->msg)?recv_msg->msg:"NULL", recv_msg->msg_size);

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
		dcm_stderror("accept failed");
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
		dcm_stderror("socket failed");
		return DCM_ERROR_NETWORK;
	}

	/* Set socket address */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	unlink(DCM_IPC_PATH[port]);
	strncpy(serv_addr.sun_path, DCM_IPC_PATH[port], sizeof(serv_addr.sun_path) - 1);

	/* Bind socket to local address */
	for (i = 0; i < 100; i++) {
		if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
			bind_success = true;
			break;
		}
		dcm_debug("#%d bind", i);
		usleep(250000);
	}

	if (bind_success == false) {
		dcm_stderror("bind failed");
		close(sock);
		return DCM_ERROR_NETWORK;
	}
	dcm_debug("bind success");

	/* Listenint */
	if (listen(sock, SOMAXCONN) < 0) {
		dcm_stderror("listen failed");
		close(sock);
		return DCM_ERROR_NETWORK;
	}
	dcm_debug("Listening...");

	/* change permission of local socket file */
	if (chmod(DCM_IPC_PATH[port], 0777) < 0)
		dcm_stderror("chmod failed");

	*socket_fd = sock;

	return DCM_SUCCESS;
}

int DcmIpcUtils::sendClientSocketMsg(int socket_fd, DcmIpcMsgType msg_type, uid_t uid, const char *msg, DcmIpcPortType port)
{
	if (port < 0 || port >= DCM_IPC_PORT_MAX) {
		dcm_error("Invalid port! Stop sending message...");
		return DCM_ERROR_INVALID_PARAMETER;
	}
	dcm_debug("Send message type: %d", msg_type);

	DcmIpcMsg send_msg;
	int sock = -1;

	if (socket_fd < 0) {
		struct sockaddr_un serv_addr;
		struct timeval tv_timeout = { 10, 0 };
		
		if ((sock = socket(PF_FILE, SOCK_STREAM, 0)) < 0) {
			dcm_stderror("socket failed");
			return DCM_ERROR_NETWORK;
		}

		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv_timeout, sizeof(tv_timeout)) == -1) {
			dcm_stderror("setsockopt failed");
			close(sock);
			return DCM_ERROR_NETWORK;
		}

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sun_family = AF_UNIX;
		strncpy(serv_addr.sun_path, DCM_IPC_PATH[port], sizeof(DCM_IPC_PATH[port]));
		
		/* Connecting to the thumbnail server */
		if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
			dcm_stderror("connect");
			close(sock);
			return DCM_ERROR_NETWORK;
		}
	} else {
		sock = socket_fd;
	}

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
		close(sock);
		return DCM_ERROR_NETWORK;
	}

	/* Send msg to the socket */
	if (send(sock, &send_msg, sizeof(send_msg), 0) != sizeof(send_msg)) {
		dcm_stderror("send failed");
		close(sock);
		return DCM_ERROR_NETWORK;
	}

	dcm_debug("Sent message type: %d", msg_type);

	close(sock);
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
		return DCM_ERROR_NETWORK;
	}

	/* Create a new TCP socket */
	if ((socket_fd = socket(PF_FILE, SOCK_STREAM, 0)) < 0) {
		dcm_stderror("socket failed");
		return DCM_ERROR_NETWORK;
	}

	/* Set dcm thread socket address */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strncpy(serv_addr.sun_path, DCM_IPC_PATH[port], sizeof(serv_addr.sun_path) - 1);

	/* Connect to the socket */
	if (connect(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		dcm_stderror("connect error");
		close(socket_fd);
		return DCM_ERROR_NETWORK;
	}

	/* Send msg to the socket */
	if (send(socket_fd, &send_msg, sizeof(send_msg), 0) != sizeof(send_msg)) {
		dcm_stderror("send failed");
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
