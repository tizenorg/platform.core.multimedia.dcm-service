/*
 *  com.samsung.dcm-svc-client
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jiyong Min <jiyong.min@samsung.com>
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
#include <glib.h>
#include <media-util.h>
#include <tzplatform_config.h>

#define MAX_STRING_LEN 4096
#define ALL "ALL"
#define SINGLE "SINGLE"

static char *g_path = NULL;
static char *g_command = NULL;

int main(int argc, char **argv)
{
	if (argc < 1) {
		printf("Usage: dcm_test ALL|SINGLE file_path \n");
		return 0;
	}

	g_command = (char *)g_malloc(MAX_STRING_LEN * sizeof(char *));
	memset(g_command, 0x00, MAX_STRING_LEN);
	snprintf(g_command, MAX_STRING_LEN, "%s", argv[1]);

	if (strcmp(g_command, SINGLE) == 0) {
		if (argc < 2) {
			printf("Usage: dcm_test ALL|SINGLE file_path \n");
			return 0;
		}

		g_path = (char *)g_malloc(MAX_STRING_LEN * sizeof(char *));
		memset(g_path, 0x00, MAX_STRING_LEN);
		snprintf(g_path, MAX_STRING_LEN, "%s", argv[2]);

		printf("dcm_test SINGLE file_path=%s \n", g_path);

		dcm_svc_request_extract_media(g_path, tzplatform_getuid(TZ_USER_NAME));
	} else if (strcmp(g_command, ALL) == 0) {
		printf("dcm_test ALL \n");

		dcm_svc_request_extract_all(tzplatform_getuid(TZ_USER_NAME));
	} else {
		printf("Usage: dcm_test ALL|SINGLE file_path \n");
		return 0;
	}

	return -1;
}
