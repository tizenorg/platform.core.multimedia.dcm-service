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

#ifndef _DCM_MAIN_SVC_H_
#define _DCM_MAIN_SVC_H_

#include <glib.h>
#include <DcmTypes.h>
#include <DcmIpcUtils.h>

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif

class DcmMainSvc {
	static DcmMainSvc *dcmMainSvc;

public:
	/* scan thread related */
	/* GMainLoop *g_dcm_svc_mainloop; */

	/* scan thread related */
	GThread *scan_thread;
	GAsyncQueue *scan_thread_ready;

	/* main thread related */
	GMainContext *main_loop_context;
	GMainContext *main_thread_context;
	GIOChannel *main_thread_recv_channel;
	int main_thread_recv_socket;
	GSource *main_thread_quit_timer;
	bool scan_thread_working;

	static DcmMainSvc *getInstance(void);
	void dcmServiceStartjobs();
	void dcmServiceFinishjobs();
	int waitScanThreadReady();
	int createScanThread();
	int createQuitTimerMainLoop();
	void quitDcmSvcMainLoop();
};

#endif /* _DCM_MAIN_SVC_H_ */

