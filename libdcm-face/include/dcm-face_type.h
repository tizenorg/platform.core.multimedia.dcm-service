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
#ifndef __TIZEN_DCM_FACE_TYPE_H__
#define __TIZEN_DCM_FACE_TYPE_H__

#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup
 * @{
 */

/**
 * @brief Facel engine handle.
 */
typedef struct face_handle_s *dcm_face_h;

/**
 * @brief Enumerations of error codes for the Facial Engine API.
 */
typedef enum {
	FACE_ERROR_NONE = TIZEN_ERROR_NONE, /**< Successful */
	FACE_ERROR_INVALID_PARAMTER = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
	FACE_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY, /**< Out of memory */
	FACE_ERROR_ENGINE_NOT_FOUND = TIZEN_ERROR_UIX_CLASS | 0x41, /**< Facial engine not found */
	FACE_ERROR_OPERATION_FAILED = TIZEN_ERROR_UIX_CLASS | 0x42, /**< Operation failed */
} face_error_e;

/**
 * @brief Enumerations of color spaces
 * @see face_image_create()
 * @see face_attr_get_prefered_colorspace()
 *
 */
typedef enum {
	FACE_IMAGE_COLORSPACE_YUV420,			/**< Y:U:V = 4:2:0 */
	FACE_IMAGE_COLORSPACE_RGB888,			/**< RGB565, high-byte is Blue 	*/
} face_image_colorspace_e;

/**
 * @brief Represents a rectangular region in a coordinate space
 */
typedef struct {
	int x;		/**< The x coordinate of the top-left corner of the rectangle */
	int y;		/**< The y coordinate of the top-left corner of the rectangle */
	int w;		/**< The width of the rectangle */
	int h;		/**< The height of the rectangle */
	int orientation;	/** face orientation */
} face_rect_s;


typedef struct {
	face_rect_s *rects;
	int count;
} face_info_s;



/**
 * @}
 */



#ifdef __cplusplus
}
#endif


#endif		/* __TIZEN_DCM_FACE_TYPE_H__ */

