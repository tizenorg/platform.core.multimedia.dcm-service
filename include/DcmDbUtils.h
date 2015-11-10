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

#ifndef _DCM_DB_UTILS_H_
#define _DCM_DB_UTILS_H_

#include <stdbool.h>
#include <sqlite3.h>
#include <glib.h>
#include <media-util.h>
/*#include <media-util-noti-face.h> */
#include <dcm-face.h>
#include <DcmTypes.h>

typedef bool (*dcm_svc_db_face_cb)(DcmFaceItem *face_item, void *user_data);

class DcmDbUtils {
private:
	DcmDbUtils();
	static DcmDbUtils *dcmDbUtils;
	static MediaDBHandle *db_handle;

public:
	uid_t dcm_uid;
	static DcmDbUtils *getInstance();
	int _dcm_svc_db_connect(uid_t uid);
	int _dcm_svc_db_disconnect();
	int _dcm_svc_db_get_scan_image_list_by_path(GList **image_list, bool mmc_mounted, const char *file_path);
	int _dcm_svc_db_get_scan_image_list_from_db(GList **image_list, bool mmc_mounted);
	int _dcm_svc_db_insert_category_to_db(void *item);
	int _dcm_svc_db_generate_uuid(DcmFaceItem **face);
	int _dcm_svc_db_insert_face_to_db(DcmFaceItem *face);
	int _dcm_svc_db_update_color_to_db(const DcmColorItem color);
	int _dcm_svc_db_insert_face_to_face_scan_list(DcmScanItem *scan_item);
	int _dcm_svc_db_check_scanned_by_media_uuid(const char *media_uuid, bool *media_scanned);
};

#endif /*_DCM_DB_UTILS_H_*/

