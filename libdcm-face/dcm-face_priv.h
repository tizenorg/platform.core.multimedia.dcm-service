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
#ifndef __TIZEN_UIX_FACE_PRIV_H__
#define __TIZEN_UIX_FACE_PRIV_H__

#include <stdlib.h>
#include <glib.h>
#include "dcm-face-debug.h"

#undef __in
#define __in

#undef __out
#define __out

#undef __inout
#define __inout

#define FACE_IMAGE_MAGIC 		(0x1a2b3c4d)
#define FACE_INVALID_MAGIC 		(0xDEADBEAF)

#define DCM_SAFE_FREE(src)	{ if(src) {free(src); src = NULL;}}

typedef struct face_image_s {
	unsigned char *data;
	unsigned int width;
	unsigned int height;
	unsigned long long size;
	face_image_colorspace_e colorspace;
	unsigned int magic;
} FaceImage;

typedef struct face_handle_s {
	void *fengine;
	unsigned int magic;
	FaceImage *image_info;
} FaceHandleT;

int _face_handle_create(void **handle);

/**
 * @brief Destroys the handle to the face engine and releases all its resources.
 * @param[in] face The face engine handle
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @retval #FACE_ERROR_ENGINE_NOT_FOUND face engine not found
 * @see _face_handle_create()
 */
int _face_handle_destroy(void *handle);

/**
 * @brief Detects faces from the image data.
 * @param[in] face The facial engine handle
 * @param[in] image_type The type of the image
 * @param[in] image The image handle to detect faces
 * @param[out] face_rect The array of the detected face positions
 * @param[out] count The number of the detected face positions
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @retval #FACE_ERROR_OUT_OF_MEMORY	Out of memory
 * @retval #FACE_ERROR_ENGINE_NOT_FOUND Facial engine not found
 */
int _face_detect_faces(dcm_face_h handle, face_rect_s *face_rect[], int *count);


#endif	/* __TIZEN_UIX_FACE_PRIV_H__ */

