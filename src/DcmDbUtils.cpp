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

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <media-util.h>
#include <tzplatform_config.h>
#include <DcmDbUtils.h>
#include <DcmTypes.h>
#include <DcmDebugUtils.h>
#include <uuid/uuid.h>

#define DCM_STRING_VALID(str)		((str != NULL && strlen(str) > 0) ? TRUE : FALSE)
#define DCM_SQLITE3_FINALIZE(x)		{if (x != NULL) {sqlite3_finalize(x);x=NULL;}}
#define DCM_SQLITE3_FREE(x)			{if (x != NULL) {sqlite3_free(x);x=NULL;}}

#define DB_TABLE_FACE "face"
#define DB_TABLE_FACE_SCAN_LIST "face_scan_list"
#define DB_TABLE_MEDIA "media"
#define FACE_ITEM "face_uuid, media_uuid, face_rect_x , face_rect_y, face_rect_w , face_rect_h, orientation"

#define SELECT_PATH_FROM_UNEXTRACTED_DCM_MEDIA "SELECT media_uuid, path, storage_uuid, width, height, orientation, mime_type FROM media WHERE media_uuid NOT IN (SELECT DISTINCT media_uuid FROM face_scan_list) AND validity=1 AND media_type=0 AND (storage_type = 0 OR storage_type = 1);"
#define SELECT_PATH_FROM_UNEXTRACTED_DCM_INTERNAL_MEDIA "SELECT media_uuid, path, storage_uuid, width, height, orientation, mime_type FROM media WHERE media_uuid NOT IN (SELECT DISTINCT media_uuid FROM face_scan_list) AND validity=1 AND media_type=0 AND storage_type=0;"

#define SELECT_MEDIA_INFO_BY_FILE_PATH_FROM_DB "SELECT media_uuid, storage_uuid, width, height, orientation, mime_type FROM media WHERE path = '%q';"
#define INSERT_FACE_ITEM_TO_DB "INSERT INTO "DB_TABLE_FACE" ("FACE_ITEM") VALUES ('%q', '%q', %d, %d, %d, %d, %d);"
#define UPDATE_COLOR_ITEM_TO_DB "UPDATE "DB_TABLE_MEDIA" SET color_r=%d, color_g=%d, color_b=%d WHERE media_uuid='%q' AND storage_uuid='%q';"

static GMutex gMutexLock;

namespace DcmDbUtilsInternal {
bool __dcm_svc_db_check_duplicated(MediaDBHandle *db_handle, DcmFaceItem *data);
bool __dcm_svc_db_check_duplicated_scan_list(MediaDBHandle *db_handle, const char *data);
static int __dcm_svc_sql_prepare_to_step(sqlite3 *handle, const char *sql_str, sqlite3_stmt** stmt);
static int __dcm_svc_sql_prepare_to_step_simple(sqlite3 *handle, const char *sql_str, sqlite3_stmt** stmt);
}

static DcmDbUtils* DcmDbUtils::getInstance(void)
{
	if (dcmDbUtils == NULL) {
		g_mutex_trylock(&gMutexLock);

		if (dcmDbUtils == NULL) {
			dcmDbUtils = new DcmDbUtils();
		}

		g_mutex_unlock(&gMutexLock);
	}

	return dcmDbUtils;
}

DcmDbUtils *DcmDbUtils::dcmDbUtils = NULL;
MediaDBHandle *DcmDbUtils::db_handle = NULL;

DcmDbUtils::DcmDbUtils(void)
{

}

bool DcmDbUtilsInternal::__dcm_svc_db_check_duplicated(MediaDBHandle *db_handle, DcmFaceItem *data)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * handle = (sqlite3 *)db_handle;
	sqlite3_stmt *sql_stmt = NULL;
	char *query_string = NULL;
	int count = 0;

	DCM_CHECK_FALSE((data != NULL));
	DCM_CHECK_FALSE((data->media_uuid != NULL));

	query_string = sqlite3_mprintf("SELECT count(*) FROM %s WHERE (media_uuid='%s' AND"
		" face_rect_x='%d' AND face_rect_y='%d' AND face_rect_w='%d' AND face_rect_h='%d' AND orientation='%d')"
		, DB_TABLE_FACE, data->media_uuid
		, data->face_rect_x, data->face_rect_y, data->face_rect_w, data->face_rect_h, data->orientation);

	ret = __dcm_svc_sql_prepare_to_step(handle, query_string, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("error when __dcm_svc_sql_prepare_to_step. ret = [%d]", ret);
		return TRUE;
	}

	count = sqlite3_column_int(sql_stmt, 0);

	DCM_SQLITE3_FINALIZE(sql_stmt);

	if (count > 0) {
		dcm_warn("duplicated face data!");
		return TRUE;
	}

	return FALSE;
}

bool DcmDbUtilsInternal::__dcm_svc_db_check_duplicated_scan_list(MediaDBHandle *db_handle, const char *data)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * handle = (sqlite3 *)db_handle;
	sqlite3_stmt *sql_stmt = NULL;
	char *query_string = NULL;
	int count = 0;

	DCM_CHECK_FALSE((data != NULL));

	query_string = sqlite3_mprintf("SELECT count(*) FROM %s WHERE media_uuid='%s'", DB_TABLE_FACE_SCAN_LIST, data);

	ret = __dcm_svc_sql_prepare_to_step(handle, query_string, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("error when __dcm_svc_sql_prepare_to_step. ret = [%d]", ret);
		return TRUE;
	}

	count = sqlite3_column_int(sql_stmt, 0);

	DCM_SQLITE3_FINALIZE(sql_stmt);

	if (count > 0) {
		dcm_warn("duplicated media data!");
		return TRUE;
	}

	return FALSE;
}


static int DcmDbUtilsInternal::__dcm_svc_sql_prepare_to_step(sqlite3 *handle, const char *sql_str, sqlite3_stmt** stmt)
{
	int err = -1;

	dcm_debug("[SQL query] : %s", sql_str);

	if (!DCM_STRING_VALID(sql_str))
	{
		dcm_error("invalid query");
		return MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	err = sqlite3_prepare_v2(handle, sql_str, -1, stmt, NULL);
	sqlite3_free((char *)sql_str);

	if (err != SQLITE_OK) {
		dcm_error ("prepare error %d[%s]", err, sqlite3_errmsg(handle));
		if (err == SQLITE_CORRUPT) {
			return MS_MEDIA_ERR_DB_CORRUPT;
		} else if (err == SQLITE_PERM) {
			return MS_MEDIA_ERR_DB_PERMISSION;
		}

		return MS_MEDIA_ERR_DB_INTERNAL;
	}

	err = sqlite3_step(*stmt);
	if (err != SQLITE_ROW) {
		dcm_error("[No-Error] Item not found. end of row [%s]", sqlite3_errmsg(handle));
		DCM_SQLITE3_FINALIZE(*stmt);
		return MS_MEDIA_ERR_DB_NO_RECORD;
	}

	return MS_MEDIA_ERR_NONE;
}

static int DcmDbUtilsInternal::__dcm_svc_sql_prepare_to_step_simple(sqlite3 *handle, const char *sql_str, sqlite3_stmt** stmt)
{
	int err = -1;

	dcm_debug("[SQL query] : %s", sql_str);

	if (!DCM_STRING_VALID(sql_str))
	{
		dcm_error("invalid query");
		return MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	err = sqlite3_prepare_v2(handle, sql_str, -1, stmt, NULL);
	sqlite3_free((char *)sql_str);

	if (err != SQLITE_OK) {
		dcm_error ("prepare error %d[%s]", err, sqlite3_errmsg(handle));
		if (err == SQLITE_CORRUPT) {
			return MS_MEDIA_ERR_DB_CORRUPT;
		} else if (err == SQLITE_PERM) {
			return MS_MEDIA_ERR_DB_PERMISSION;
		}

		return MS_MEDIA_ERR_DB_INTERNAL;
	}

	return MS_MEDIA_ERR_NONE;
}

int DcmDbUtils::_dcm_svc_db_connect(uid_t uid)
{
	int err = -1;

	dcm_debug("_dcm_svc_db_connect uid: %d", uid);
	dcm_uid = uid;

	err = media_db_connect(&db_handle, dcm_uid, TRUE);
	if (err != MS_MEDIA_ERR_NONE) {
		dcm_error("media_db_connect failed: %d", err);
		db_handle = NULL;
		return err;
	}

	dcm_warn("media db handle: %p", db_handle);

	dcm_debug_fleave();

	return MS_MEDIA_ERR_NONE;
}

int DcmDbUtils::_dcm_svc_db_disconnect(void)
{
	int err = -1;

	dcm_warn("media db handle: %p", db_handle);

	if (db_handle != NULL) {
		err = media_db_disconnect(db_handle);
		if (err != MS_MEDIA_ERR_NONE) {
			dcm_error("media_db_disconnect failed: %d", err);
			db_handle = NULL;
			return err;
		}
	}

	db_handle = NULL;

	dcm_debug_fleave();

	return MS_MEDIA_ERR_NONE;
}


int DcmDbUtils::_dcm_svc_db_get_scan_image_list_by_path(GList **image_list, bool mmc_mounted, const char *file_path)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *query_string = NULL;
	sqlite3_stmt *sql_stmt = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(db_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(file_path, DCM_ERROR_INVALID_PARAMETER);

	/* Make query */
	if (mmc_mounted == true) {
		query_string = sqlite3_mprintf(SELECT_MEDIA_INFO_BY_FILE_PATH_FROM_DB, file_path);
	} else {
		query_string = sqlite3_mprintf(SELECT_MEDIA_INFO_BY_FILE_PATH_FROM_DB, file_path);
	}

	if (query_string == NULL) {
		dcm_error("Failed to make query!");
		return MS_MEDIA_ERR_OUT_OF_MEMORY;
	}

	ret = DcmDbUtilsInternal::__dcm_svc_sql_prepare_to_step_simple((sqlite3 *)db_handle, query_string, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("error when __dcm_svc_sql_prepare_to_step. ret = [%d]", ret);
		return TRUE;
	}

	
	while(sqlite3_step(sql_stmt) == SQLITE_ROW) {
		DcmScanItem *scan_item = (DcmScanItem *) g_malloc0(sizeof(DcmScanItem));
		if (!scan_item) {
			dcm_error("Failed to allocate memory for scan_item!");
			continue;
		}

		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 0)))
			scan_item->media_uuid = strdup((const char *)sqlite3_column_text(sql_stmt, 0));

		if (DCM_STRING_VALID(file_path))
			scan_item->file_path = strdup(file_path);

		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1)))
			scan_item->storage_uuid = strdup((const char *)sqlite3_column_text(sql_stmt, 1));

		scan_item->image_width = sqlite3_column_int(sql_stmt, 2);
		scan_item->image_height = sqlite3_column_int(sql_stmt, 3);
		scan_item->image_orientation = sqlite3_column_int(sql_stmt, 4);
		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 5)))
			scan_item->mime_type = strdup((const char *)sqlite3_column_text(sql_stmt, 5));

		/* scan item retrieved by this function will be marked as SCAN_SINGLE */
		scan_item->scan_item_type = DCM_SCAN_ITEM_TYPE_SCAN_SINGLE;

		*image_list = g_list_append(*image_list, scan_item);

		dcm_sec_debug("media uuid: [%s] file path: [%s]", scan_item->media_uuid, scan_item->file_path);
	}
	

	DCM_SQLITE3_FINALIZE(sql_stmt);

	dcm_debug_fleave();

	return ret;
}

int DcmDbUtils::_dcm_svc_db_get_scan_image_list_from_db(GList **image_list, bool mmc_mounted)
{
	int ret = MS_MEDIA_ERR_NONE;
	char * query_string = NULL;
	sqlite3_stmt *sql_stmt = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(db_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(image_list, DCM_ERROR_INVALID_PARAMETER);

	/* Make query */
	if (mmc_mounted == true) {
		query_string = sqlite3_mprintf(SELECT_PATH_FROM_UNEXTRACTED_DCM_MEDIA);
	} else {
		query_string = sqlite3_mprintf(SELECT_PATH_FROM_UNEXTRACTED_DCM_INTERNAL_MEDIA);
	}

	ret = DcmDbUtilsInternal::__dcm_svc_sql_prepare_to_step_simple((sqlite3 *)db_handle, query_string, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("error when __dcm_svc_sql_prepare_to_step_simple. ret = [%d]", ret);
		return DCM_ERROR_DB_OPERATION;
	}

	while(sqlite3_step(sql_stmt) == SQLITE_ROW) {
		DcmScanItem *scan_item = (DcmScanItem *) g_malloc0(sizeof(DcmScanItem));
		if (!scan_item) {
			dcm_error("Failed to allocate memory for scan_item!");
			continue;
		}

		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 0)))
			scan_item->media_uuid = strdup((const char *)sqlite3_column_text(sql_stmt, 0));

		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1)))
			scan_item->file_path = strdup((const char *)sqlite3_column_text(sql_stmt, 1));

		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 2)))
			scan_item->storage_uuid = strdup((const char *)sqlite3_column_text(sql_stmt, 2));

		scan_item->image_width = sqlite3_column_int(sql_stmt, 3);
		scan_item->image_height = sqlite3_column_int(sql_stmt, 4);
		scan_item->image_orientation = sqlite3_column_int(sql_stmt, 5);
		if (DCM_STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 6)))
			scan_item->mime_type = strdup((const char *)sqlite3_column_text(sql_stmt, 6));

		/* scan item retrieved by this function will be marked as SCAN_ALL */
		scan_item->scan_item_type = DCM_SCAN_ITEM_TYPE_SCAN_ALL;

		*image_list = g_list_append(*image_list, scan_item);

		dcm_sec_debug("media uuid: [%s] file path: [%s]", scan_item->media_uuid, scan_item->file_path);
	}

	DCM_SQLITE3_FINALIZE(sql_stmt);

	dcm_debug_fleave();

	return MS_MEDIA_ERR_NONE;
}

int DcmDbUtils::_dcm_svc_db_generate_uuid(DcmFaceItem **face)
{
	int ret = MS_MEDIA_ERR_NONE;
	uuid_t uuid_value;
	static char uuid_unparsed[50] = {0, };

	dcm_debug_fenter();
	DCM_CHECK_VAL(face, DCM_ERROR_INVALID_PARAMETER);

	uuid_generate(uuid_value);
	uuid_unparse(uuid_value, uuid_unparsed);

	(*face)->face_uuid = strdup(uuid_unparsed);

	if ((*face)->face_uuid == NULL) {
		ret = DCM_ERROR_UUID_GENERATE_FAILED;
	} else {
		dcm_debug("set face_uuid :%s", (*face)->face_uuid);
	}

	dcm_debug_fleave();

	return ret;
}

int DcmDbUtils::_dcm_svc_db_insert_face_to_db(DcmFaceItem *face)
{
	int ret = DCM_SUCCESS;
	int err = MS_MEDIA_ERR_NONE;
	char* query_string = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(db_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(face, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(face->face_uuid, DCM_ERROR_INVALID_PARAMETER);

	if (DcmDbUtilsInternal::__dcm_svc_db_check_duplicated(db_handle, face) == TRUE) {
		dcm_error("[__dcm_svc_db_check_duplicated] The data is duplicated!");
		return DCM_ERROR_DUPLICATED_DATA;
	}

	query_string = sqlite3_mprintf(INSERT_FACE_ITEM_TO_DB, face->face_uuid, face->media_uuid, face->face_rect_x, face->face_rect_y, face->face_rect_w, face->face_rect_h, face->orientation);

	dcm_debug("query is %s", query_string);

	g_mutex_trylock(&gMutexLock);
	err = media_db_request_update_db(query_string, dcm_uid);
	if (err != MS_MEDIA_ERR_NONE) {
		dcm_error("media_db_request_update_db fail = %d, %s", ret, sqlite3_errmsg((sqlite3 *)db_handle));
		ret = DCM_ERROR_DB_OPERATION;
	}
	g_mutex_unlock(&gMutexLock);

	DCM_SQLITE3_FREE(query_string);

	dcm_debug_fleave();

	return ret;
}

int DcmDbUtils::_dcm_svc_db_insert_face_to_face_scan_list(DcmScanItem *scan_item)
{
	int ret = DCM_SUCCESS;
	int err = MS_MEDIA_ERR_NONE;
	char* query_string = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(db_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(scan_item, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(scan_item->media_uuid, DCM_ERROR_INVALID_PARAMETER);

	if (DcmDbUtilsInternal::__dcm_svc_db_check_duplicated_scan_list(db_handle, scan_item->media_uuid) == TRUE) {
		dcm_error("[_dcm_svc_db_insert_face_to_face_scan_list] The data is duplicated!");
		return DCM_ERROR_DUPLICATED_DATA;
	}

	query_string = sqlite3_mprintf("INSERT INTO %s (media_uuid, storage_uuid) values('%q', '%q')", DB_TABLE_FACE_SCAN_LIST, scan_item->media_uuid, scan_item->storage_uuid);

	dcm_debug("query is %s", query_string);

	g_mutex_trylock(&gMutexLock);
	err = media_db_request_update_db(query_string, dcm_uid);
	if (err != MS_MEDIA_ERR_NONE) {
		dcm_error("media_db_request_update_db is failed: %d, %s", ret, sqlite3_errmsg((sqlite3 *)db_handle));
		ret = DCM_ERROR_DB_OPERATION;
	}
	g_mutex_unlock(&gMutexLock);

	DCM_SQLITE3_FREE(query_string);

	dcm_debug_fleave();

	return ret;
}

int DcmDbUtils::_dcm_svc_db_update_color_to_db(DcmColorItem color)
{
	int ret = DCM_SUCCESS;

	dcm_debug_fenter();
#if 0
	int err = MS_MEDIA_ERR_NONE;
	char* query_string = NULL;

	DCM_CHECK_VAL(db_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(color.media_uuid, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(color.storage_uuid, DCM_ERROR_INVALID_PARAMETER);

	query_string = sqlite3_mprintf(UPDATE_COLOR_ITEM_TO_DB, (int)(color.rgb_r), (int)(color.rgb_g), (int)(color.rgb_b), color.media_uuid, color.storage_uuid);
	dcm_debug("query is %s", query_string);

	g_mutex_trylock(&gMutexLock);
	err = media_db_request_update_db(query_string, dcm_uid);
	if (err != MS_MEDIA_ERR_NONE) {
		dcm_error("media_db_request_update_db fail = %d, %s", ret, sqlite3_errmsg((sqlite3 *)db_handle));
		ret = DCM_ERROR_DB_OPERATION;
	}
	g_mutex_unlock(&gMutexLock);

	DCM_SQLITE3_FREE(query_string);
#endif
	dcm_debug_fleave();

	return ret;
}

int DcmDbUtils::_dcm_svc_db_check_scanned_by_media_uuid(const char *media_uuid, bool *media_scanned)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *query_string = NULL;
	sqlite3_stmt *sql_stmt = NULL;
	int count = 0;

	dcm_debug_fenter();

	DCM_CHECK_VAL(db_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(media_uuid, DCM_ERROR_INVALID_PARAMETER);

	query_string = sqlite3_mprintf("SELECT count(*) FROM %s WHERE (media_uuid='%q')", DB_TABLE_FACE_SCAN_LIST, media_uuid);
	DCM_CHECK_VAL(query_string, DCM_ERROR_OUT_OF_MEMORY);

	ret = DcmDbUtilsInternal::__dcm_svc_sql_prepare_to_step((sqlite3 *)db_handle, query_string, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		dcm_error("error when __dcm_svc_sql_prepare_to_step. ret = [%d]", ret);
		return DCM_ERROR_DB_OPERATION;
	}

	count = sqlite3_column_int(sql_stmt, 0);

	DCM_SQLITE3_FINALIZE(sql_stmt);

	if (count > 0)
		*media_scanned = TRUE;
	else
		*media_scanned = FALSE;

	dcm_debug_fleave();

	return ret;
}
