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

#ifndef _DCM_SVC_IPC_H_
#define _DCM_SVC_IPC_H_

#include <DcmTypes.h>

namespace DcmIpcUtils {
	int createSocket(int *socket_fd, DcmIpcPortType port);
	int acceptSocket(int serv_sock, int *client_sock);
	int receiveSocketMsg(int client_sock, DcmIpcMsg *recv_msg);
	int sendSocketMsg(DcmIpcMsgType msg_type, uid_t uid, const char *msg, DcmIpcPortType port);
	int closeSocket(int socket_fd);
	int createUDPSocket(int *socket_fd);
	int receiveUDPSocketMsg(int socket_fd, DcmIpcMsg *recv_msg);
	int sendUDPSocketMsg(DcmIpcMsgType msg_type, const char *msg);
	int closeUDPSocket(int socket_fd);
	int sendFaceMsg(const char *media_uuid);
}

#endif /* _DCM_SVC_IPC_H_ */

