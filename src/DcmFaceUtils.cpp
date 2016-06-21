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
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <glib.h>
#include <image_util.h>
#include <dcm-face.h>
#include <dcm_image_codec.h>
#include <DcmDbUtils.h>
#include <DcmTypes.h>
#include <DcmDebugUtils.h>
#include <DcmFaceUtils.h>


namespace DcmFaceApi {
int createFaceItem(DcmFaceItem **face);
double caculateScaleFactor(DcmImageInfo *image_info);
void freeDcmFaceItem(void *data);
}

static dcm_face_h dcm_face_handle = NULL;

int DcmFaceUtils::initialize()
{
	int ret = FACE_ERROR_NONE;

	dcm_debug_fenter();

	ret = dcm_face_create(&dcm_face_handle);
	if (ret != FACE_ERROR_NONE) {
		dcm_error("Failed to dcm_face_create err: %d", ret);
		return DCM_ERROR_FACE_ENGINE_FAILED;
	}

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

int DcmFaceUtils::finalize()
{
	int ret = FACE_ERROR_NONE;

	dcm_debug_fenter();

	DCM_CHECK_VAL(dcm_face_handle, DCM_ERROR_INVALID_PARAMETER);

	ret = dcm_face_destroy(dcm_face_handle);
	if (ret != FACE_ERROR_NONE) {
		dcm_error("Failed to dcm_face_destroy ret: %d", ret);
		return DCM_ERROR_FACE_ENGINE_FAILED;
	}

	dcm_face_handle = NULL;

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

int DcmFaceUtils::runFaceRecognizeProcess(DcmScanItem *scan_item, DcmImageInfo *image_info)
{
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();
	DcmFaceItem *face = NULL;
	int face_area = 0;
	int i = 0;
	double scale_factor = 0.0;
	int err = FACE_ERROR_NONE;
	int ret = DCM_SUCCESS;
	face_info_s *face_info = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(dcm_face_handle, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(scan_item, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(scan_item->media_uuid, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(image_info, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(image_info->pixel, DCM_ERROR_INVALID_PARAMETER);

	dcm_debug("colorspce: [%d], w: [%d], h: [%d]", image_info->decode_type, image_info->buffer_width, image_info->buffer_height);

	err = dcm_face_set_image_info(dcm_face_handle, (face_image_colorspace_e)image_info->decode_type, image_info->pixel, image_info->buffer_width, image_info->buffer_height, image_info->size);
	if (err != FACE_ERROR_NONE) {
		dcm_error("Failed to dcm_face_set_image_info! err: %d", err);
		ret = DCM_ERROR_FACE_ENGINE_FAILED;
		goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
	}

	face_info = (face_info_s *)g_malloc0(sizeof(face_info_s));
	if (face_info == NULL) {
		dcm_error("Failed to allocate face info");
		ret = DCM_ERROR_OUT_OF_MEMORY;
		goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
	}

	err = dcm_face_get_face_info(dcm_face_handle, face_info);
	if (err != FACE_ERROR_NONE) {
		dcm_error("Failed to get face info! err: %d", err);
		ret = DCM_ERROR_FACE_ENGINE_FAILED;
		goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
	}

	dcm_warn("detected face count: %d", face_info->count);
	if (face_info->count <= 0) {
		scan_item->face_count = 0;
		goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
	}
	scan_item->face_count = face_info->count;

	/* Compute scale factor between decode size and original size */
	scale_factor = DcmFaceApi::caculateScaleFactor(image_info);

	/* Insert every face rectangle into database */
	for (i = 0; i < face_info->count; i++) {
		face = NULL;

		err = DcmFaceApi::createFaceItem(&face);
		if (err != DCM_SUCCESS) {
			dcm_error("Failed to create face items! err: %d", err);
			ret = err;
			goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
		}

		if (scale_factor > 1.0) {
			face->face_rect_x = (int) (face_info->rects[i].x * scale_factor);
			face->face_rect_y = (int) (face_info->rects[i].y * scale_factor);
			face->face_rect_w = (int) (face_info->rects[i].w * scale_factor);
			face->face_rect_h = (int) (face_info->rects[i].h * scale_factor);
		} else {
			face->face_rect_x = face_info->rects[i].x;
			face->face_rect_y = face_info->rects[i].y;
			face->face_rect_w = face_info->rects[i].w;
			face->face_rect_h = face_info->rects[i].h;
		}
		face->orientation = face_info->rects[i].orientation;

		face_area += face->face_rect_w * face->face_rect_h;
		dcm_debug("[#%d] face rect: XYWH (%d, %d, %d, %d)", i, face->face_rect_x, face->face_rect_y, face->face_rect_w,
		face->face_rect_h);

		face->media_uuid = strdup(scan_item->media_uuid);

		/* Insert face rectangle into database */
		err = dcmDbUtils->_dcm_svc_db_generate_uuid(&face);
		if (err != DCM_SUCCESS) {
			dcm_error("Failed to set uuid! err: %d", err);
			goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
		}
		err = dcmDbUtils->_dcm_svc_db_insert_face_to_db(face);
		if (err != DCM_SUCCESS) {
			dcm_error("Failed to insert face item into db! err: %d", err);
			goto DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED;
		}

		/* Send db updated notification */
		DcmFaceApi::freeDcmFaceItem(face);
		face = NULL;
	}

DCM_SVC_FACE_RECOGNIZE_BUFFER_FAILED:

	err = dcmDbUtils->_dcm_svc_db_insert_face_to_face_scan_list(scan_item);
	if (err != DCM_SUCCESS) {
		dcm_error("Failed to insert face item into face_scan_list! err: %d", err);
	}

	if (face_info != NULL && face_info->count > 0) {
		dcm_face_destroy_face_info(face_info);
	}

	if (face != NULL) {
		DcmFaceApi::freeDcmFaceItem(face);
		face = NULL;
	}

	dcm_debug_fleave();

	return ret;
}

int DcmFaceApi::createFaceItem(DcmFaceItem **face)
{
	DCM_CHECK_VAL(face, DCM_ERROR_INVALID_PARAMETER);

	*face = NULL;

	DcmFaceItem *_face = (DcmFaceItem*)g_malloc0(sizeof(DcmFaceItem));
	if (_face == NULL)
		return DCM_ERROR_OUT_OF_MEMORY;

	*face = _face;

	return DCM_SUCCESS;
}

double DcmFaceApi::caculateScaleFactor(DcmImageInfo *image_info)
{
	double scale_factor = 0.0;

	DCM_CHECK_VAL(image_info, 0.0);

	if (image_info->original_width >= image_info->original_height) {
		if (image_info->buffer_width >= image_info->buffer_height) {
			scale_factor = ((double) (image_info->original_width)) / ((double) (image_info->buffer_width));
		} else {
			scale_factor = ((double) (image_info->original_width)) / ((double) (image_info->buffer_height));
		}
	} else {
		if (image_info->buffer_height >= image_info->buffer_width) {
			scale_factor = ((double) (image_info->original_height)) / ((double) (image_info->buffer_height));
		} else {
			scale_factor = ((double) (image_info->original_height)) / ((double) (image_info->buffer_width));
		}
	}

	dcm_debug("scale_factor: %lf", scale_factor);

	return scale_factor;
}

void DcmFaceApi::freeDcmFaceItem(void *data)
{
	DcmFaceItem *_face = (DcmFaceItem *)data;
	DCM_CHECK(_face);

	DCM_SAFE_FREE(_face->face_uuid);
	DCM_SAFE_FREE(_face->media_uuid);
	DCM_SAFE_FREE(_face);

	return;
}
