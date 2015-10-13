/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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
 */

#include <stdbool.h>
#include <mv_common.h>
#include <mv_face.h>
#include <mv_face_type.h>

#include "dcm-face-debug.h"
#include "dcm-face.h"
#include "dcm-face_priv.h"

typedef struct media_vision_h {
	mv_engine_config_h cfg;
	mv_source_h source;
} mv_handle;

typedef struct media_vision_result_s {
	face_error_e error;
	face_rect_s *face_rect;
	int count;
} mv_faceInfo;

static face_error_e __convert_to_mv_error_e(mv_error_e err)
{
	if (err != MEDIA_VISION_ERROR_NONE)
		dcm_error("Error from media vision : %d", err);

	switch (err) {
	case MEDIA_VISION_ERROR_NONE:
		return FACE_ERROR_NONE;

	case MEDIA_VISION_ERROR_NOT_SUPPORTED:
		return FACE_ERROR_INVALID_PARAMTER;

	case MEDIA_VISION_ERROR_INVALID_PARAMETER:
		return FACE_ERROR_INVALID_PARAMTER;

	case MEDIA_VISION_ERROR_NOT_SUPPORTED_FORMAT:
		return FACE_ERROR_INVALID_PARAMTER;

	case MEDIA_VISION_ERROR_OUT_OF_MEMORY:
		return FACE_ERROR_OUT_OF_MEMORY;

	default:
		break;
	}

	return FACE_ERROR_OPERATION_FAILED;
}

static int __convert_to_mv_colorspace_e(face_image_colorspace_e src, mv_colorspace_e *dst)
{
	switch (src) {
	case FACE_IMAGE_COLORSPACE_YUV420:
		*dst = MEDIA_VISION_COLORSPACE_I420;
		return FACE_ERROR_NONE;
	case FACE_IMAGE_COLORSPACE_RGB888:
		*dst = MEDIA_VISION_COLORSPACE_RGB888;
		return FACE_ERROR_NONE;
	default:
		*dst = MEDIA_VISION_COLORSPACE_INVALID;
		dcm_error("Unknown colorspace : %d", src);
		break;
	}

	return FACE_ERROR_INVALID_PARAMTER;
}

void __face_detected_cb(mv_source_h source, mv_engine_config_h cfg, mv_rectangle_s *faces_locations, int number_of_faces, void *user_data)
{
	mv_faceInfo* _data = (mv_faceInfo*)user_data;

	dcm_error("[No-Error] number of faces: %d", number_of_faces);

	if (number_of_faces == 0) {
		_data->face_rect = NULL;
		_data->count = 0;
		_data->error = FACE_ERROR_NONE;
		return;
	}

	_data->face_rect = (face_rect_s *)calloc(number_of_faces, sizeof(face_rect_s));
	if (_data->face_rect == NULL) {
		dcm_error("Cannout allocate face_rect_s");
		_data->face_rect = NULL;
		_data->count = 0;
		_data->error = FACE_ERROR_OUT_OF_MEMORY;
		return;
	}

	int i = 0;

	for (i = 0; i < number_of_faces ; i++) {
		_data->face_rect[i].x = faces_locations[i].point.x;
		_data->face_rect[i].y = faces_locations[i].point.y;
		_data->face_rect[i].w = faces_locations[i].width;
		_data->face_rect[i].h = faces_locations[i].height;
		_data->face_rect[i].orientation = 0;	/* set to default orientation */
	}

	_data->count = number_of_faces;
	_data->error = FACE_ERROR_NONE;

	return;
}

int _face_handle_create(__inout void **handle)
{
	int err = 0;
	mv_handle* _handle = (mv_handle*)calloc(1, sizeof(mv_handle));

	dcm_debug_fenter();

	dcm_retvm_if(handle == NULL, FACE_ERROR_OUT_OF_MEMORY, "handle create fail");

	err = mv_create_engine_config(&(_handle->cfg));
	if (err != MEDIA_VISION_ERROR_NONE) {
		dcm_error("fail to mv_create_engine_config");
		DCM_SAFE_FREE(_handle);
		return __convert_to_mv_error_e(err);
	}

	err = mv_create_source(&(_handle->source));
	if (err != MEDIA_VISION_ERROR_NONE) {
		dcm_error("fail to mv_create_source");
		mv_destroy_engine_config(_handle->cfg);
		DCM_SAFE_FREE(_handle);
		return __convert_to_mv_error_e(err);
	}

	*handle = _handle;

	dcm_info("dcm_face_engine was created. handle=0x%08x", *handle);

	return FACE_ERROR_NONE;
}

int _face_handle_destroy(__in void *handle)
{
	int err = 0;

	mv_handle* _handle = (mv_handle*)handle;

	dcm_info("dcm_face_engine destroy. handle=0x%08x", handle);

	err = mv_destroy_engine_config(_handle->cfg);
	if (err != MEDIA_VISION_ERROR_NONE) {
		dcm_error("Fail to mv_destroy_engine_config");
		return __convert_to_mv_error_e(err);
	}

	err = mv_destroy_source(_handle->source);
	if (err != MEDIA_VISION_ERROR_NONE) {
		dcm_error("Fail to mv_destroy_source");
		return __convert_to_mv_error_e(err);
	}

	return FACE_ERROR_NONE;
}

int _face_detect_faces(__in dcm_face_h handle, __out face_rect_s *face_rect[], __out int *count)
{
	int err = FACE_ERROR_NONE;
	mv_colorspace_e colorspace = MEDIA_VISION_COLORSPACE_INVALID;
	mv_faceInfo result;

	dcm_retvm_if(handle == NULL, FACE_ERROR_OUT_OF_MEMORY, "invalid handle");
	dcm_retvm_if(face_rect == NULL, FACE_ERROR_OUT_OF_MEMORY, "invalid face_rect");
	dcm_retvm_if(count == NULL, FACE_ERROR_OUT_OF_MEMORY, "invalid count");

	mv_handle *_fengine = (mv_handle *)handle->fengine;

	__convert_to_mv_colorspace_e(handle->image_info->colorspace, &colorspace);

	dcm_debug("face_detect image: %p, size: %d, width: %d, height: %d, color: %d", handle->image_info->data, handle->image_info->size, handle->image_info->width, handle->image_info->height, colorspace);

	err = mv_source_fill_by_buffer(_fengine->source, handle->image_info->data, handle->image_info->size, handle->image_info->width, handle->image_info->height, colorspace);
	if (err != MEDIA_VISION_ERROR_NONE) {
		dcm_error("Fail to mv_source_fill_by_buffer");
		return __convert_to_mv_error_e(err);
	}

	err = mv_face_detect(_fengine->source, _fengine->cfg, (mv_face_detected_cb)__face_detected_cb, &result);

/*	wait_for_async(); */

	if (result.error == FACE_ERROR_NONE) {
		*face_rect = result.face_rect;
		*count = result.count;
	} else {
		*face_rect = result.face_rect;
		*count = result.count;
		return result.error;
	}

	return FACE_ERROR_NONE;
}
