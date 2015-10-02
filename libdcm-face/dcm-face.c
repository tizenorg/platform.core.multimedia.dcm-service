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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcm-face.h"
#include "dcm-face_priv.h"


#define FACE_MAGIC_VALID 	(0xFF993311)
#define FACE_MAGIC_INVALID 	(0x393A3B3C)

EXPORT_API int dcm_face_create(__inout dcm_face_h *handle)
{
	int ret = FACE_ERROR_NONE;

	dcm_retvm_if (handle == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid handle");

	FaceHandleT *pFaceHandle = (FaceHandleT *)calloc(1, sizeof(FaceHandleT));
	dcm_retvm_if (pFaceHandle == NULL, FACE_ERROR_OUT_OF_MEMORY, "malloc fail");

	ret = _face_handle_create(&(pFaceHandle->fengine));
	dcm_retvm_if (ret != FACE_ERROR_NONE, ret, "fail to _face_handle_create");

	pFaceHandle->magic = FACE_MAGIC_VALID;

	*handle = pFaceHandle;

	dcm_info("face created. handle=0x%08x", *handle);

	return ret;
}

EXPORT_API int dcm_face_destroy(__in dcm_face_h handle)
{
	int ret = FACE_ERROR_NONE;
	dcm_info("face destroy. handle=0x%08x", handle);

	dcm_retvm_if (handle == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid handle");

	ret = _face_handle_destroy(handle->fengine);
	if (ret != FACE_ERROR_NONE) {
		dcm_error("fail to _face_handle_destroy");
		return ret;
	}

	handle->magic = FACE_MAGIC_INVALID;

	if (handle->image_info != NULL)
		DCM_SAFE_FREE(handle->image_info->data);
	DCM_SAFE_FREE(handle->image_info);
	DCM_SAFE_FREE(handle);

	return ret;
}

EXPORT_API int dcm_face_get_prefered_colorspace(__in dcm_face_h handle, __out face_image_colorspace_e *colorspace)
{
	dcm_retvm_if (handle == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid handle");
	dcm_retvm_if (colorspace == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid colorspace");

	*colorspace = FACE_IMAGE_COLORSPACE_RGB888;

	return FACE_ERROR_NONE;
}

EXPORT_API int dcm_face_set_image_info(dcm_face_h handle, face_image_colorspace_e colorspace, unsigned char *buffer, unsigned int width, unsigned int height, unsigned int size)
{
	FaceHandleT *_handle = (FaceHandleT *)handle;
	unsigned char *data = NULL;

	dcm_debug_fenter();

	dcm_retvm_if (handle == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid handle");
	dcm_retvm_if (buffer == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid buffer");

	switch (colorspace) {
		case FACE_IMAGE_COLORSPACE_YUV420:
		case FACE_IMAGE_COLORSPACE_RGB888:
			data = (unsigned char *)calloc(1, size);
			memcpy(data, buffer, size);
			break;
		default:
			dcm_error("Invalid colorspace : [%d]", colorspace);
			return FACE_ERROR_INVALID_PARAMTER;
	}

	if (_handle->image_info != NULL)
		DCM_SAFE_FREE(_handle->image_info->data);
	DCM_SAFE_FREE(_handle->image_info);

	_handle->image_info = (FaceImage *)calloc(1, sizeof(FaceImage));

	if (_handle->image_info == NULL) {
		dcm_error("Out of memory");
		DCM_SAFE_FREE(data);
		return FACE_ERROR_OUT_OF_MEMORY;
	}

	_handle->image_info->data = data;
	_handle->image_info->width = width;
	_handle->image_info->height = height;
	_handle->image_info->size = size;
	_handle->image_info->colorspace = colorspace;
	_handle->image_info->magic = FACE_IMAGE_MAGIC;

	dcm_debug_fleave();

	return FACE_ERROR_NONE;
}

EXPORT_API int dcm_face_get_face_info(__in dcm_face_h handle, __out face_info_s *face_info)
{
	int ret = FACE_ERROR_NONE;

	dcm_debug_fenter();

	dcm_retvm_if (handle == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid handle");
	dcm_retvm_if (face_info == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid face_info");

	ret = _face_detect_faces(handle, &(face_info->rects), &(face_info->count));
	if (ret != FACE_ERROR_NONE) {
		dcm_error("fail to _face_detect_faces");
	}

	dcm_debug_fleave();

	return ret;
}

EXPORT_API int dcm_face_destroy_face_info(face_info_s *face_info)
{
	dcm_retvm_if (face_info == NULL, FACE_ERROR_INVALID_PARAMTER, "Invalid face_info");

	DCM_SAFE_FREE(face_info->rects);
	DCM_SAFE_FREE(face_info);

	return FACE_ERROR_NONE;
}
