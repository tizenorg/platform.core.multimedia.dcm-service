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
#ifndef __TIZEN_DCM_FACE_H__
#define __TIZEN_DCM_FACE_H__

#include <tizen.h>
#include "dcm-face_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup DCM_FACE_MODULE
 * @{
 */

/**
 * @brief Creates a handle to the facial engine.
 * @remarks The @a face must be released with face_destroy() by you.
 * @param[out] handle A face engine handle to be newly created on success
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE	Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @retval #FACE_ERROR_OUT_OF_MEMORY	Out of memory
 * @retval #FACE_ERROR_ENGINE_NOT_FOUND Facial engine not found
 * @see dcm_face_destroy()
 */
int dcm_face_create(dcm_face_h *handle);

/**
 * @brief Destroys the handle to the facial engine and releases all its resources.
 * @param[in] handle The face engine handle
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @retval #FACE_ERROR_ENGINE_NOT_FOUND Facial engine not found
 * @see dcm_face_create()
 */
int dcm_face_destroy(dcm_face_h handle);

/**
 * @brief Gets the preferred color space of the facial engine.
 * @remarks The preferred color space depends on the facial engine.
 * @param[in] handle The facial engine handle
 * @param[out] colorspace The preferred color space
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @see dcm_face_create()
 */
int dcm_face_get_prefered_colorspace(dcm_face_h handle, face_image_colorspace_e *colorspace);

/**
 * @brief Creates a face image handle containing the input image data
 * @remarks The @a face_image must be released with face_image_destroy() by you.
 * @remarks The @a buffer must not be released until @a face_image is released with face_image_destroy()
 * @param[in] handle The facial engine handle
 * @param[in] colorspace The colorspace of the image
 * @param[in] buffer The buffer containing the image
 * @param[in] width The width of the image
 * @param[in] height The height of the image
 * @param[in] size The size of the buffer
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE	Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @retval #FACE_ERROR_OUT_OF_MEMORY	Out of memory
 * @see dcm_face_create()
 */
int dcm_face_set_image_info(dcm_face_h handle, face_image_colorspace_e colorspace, unsigned char *buffer, unsigned int width, unsigned int height, unsigned int size);

/**
 * @brief Gets the face information of detected faces.
 * @remarks The detected faces depends on the facial engine.
 * @param[in] handle The facial engine handle
 * @param[out] face info The face information with rect, count and so on
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @see dcm_face_destroy_face_info()
 */
int dcm_face_get_face_info(dcm_face_h handle, face_info_s *face_info);

/**
 * @brief Release the face information memory.
 * @param[in] face_info The face information memory
 * @return 0 on success, otherwise a negative error value.
 * @retval #FACE_ERROR_NONE Successful
 * @retval #FACE_ERROR_INVALID_PARAMTER Invalid parameter
 * @see dcm_face_get_face_info()
 */
int dcm_face_destroy_face_info(face_info_s face_info);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif


#endif		/* __TIZEN_DCM_FACE_H__ */

