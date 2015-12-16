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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <image_util.h>
#include <mm_util_imgp.h>
#include <dcm_image_debug_utils.h>
#include <dcm_image_codec.h>
#include <DcmTypes.h>

#define MIME_TYPE_JPEG "image/jpeg"
#define MIME_TYPE_PNG "image/png"
#define MIME_TYPE_BMP "image/bmp"

#define OPT_IMAGE_WIDTH		2048
#define OPT_IMAGE_HEIGHT	1536

void __dcm_decode_calc_image_size(unsigned int src_width, unsigned int src_height, unsigned int *calc_width, unsigned int *calc_height)
{
	*calc_width = 0;
	*calc_height = 0;

	if (src_width <= OPT_IMAGE_WIDTH && src_height <= OPT_IMAGE_HEIGHT) {
		dcm_debug("Original image is smaller than target image");
		*calc_width = src_width;
		*calc_height = src_height;
		return;
	}

	if (src_width > src_height) {
		*calc_width = OPT_IMAGE_WIDTH;
		*calc_height = (int)(src_height * (((double) OPT_IMAGE_WIDTH) / ((double) src_width)));
	} else {
		*calc_width = (int)(src_width * (((double) OPT_IMAGE_HEIGHT) / ((double) src_height)));
		*calc_height = OPT_IMAGE_HEIGHT;
	}
}

static int _dcm_rotate_image(const unsigned char *source, unsigned char **image_buffer, unsigned int *buff_width, unsigned int *buff_height, dcm_image_codec_type_e decode_type, int orientation)
{
	int ret = IMAGE_UTIL_ERROR_NONE;
	image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
	image_util_rotation_e rotate = IMAGE_UTIL_ROTATION_NONE;
	unsigned char *rotated_buffer = NULL;
	unsigned int rotated_buffer_size = 0;
	int rotated_width = 0, rotated_height = 0;

	DCM_CHECK_VAL(source, DCM_ERROR_INVALID_PARAMETER);

	if (decode_type == DCM_IMAGE_CODEC_I420) {
		colorspace = IMAGE_UTIL_COLORSPACE_I420;
	} else if (decode_type == DCM_IMAGE_CODEC_RGB) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
	} else if (decode_type == DCM_IMAGE_CODEC_RGBA) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
	} else {
		return DCM_ERROR_UNSUPPORTED_IMAGE_TYPE;
	}

	/* Get rotate angle enum */
	if (orientation == DEGREE_180) { // 3-180
		rotate = IMAGE_UTIL_ROTATION_180;
	} else if (orientation == DEGREE_90) { // 6-90
		rotate = IMAGE_UTIL_ROTATION_90;
	} else if (orientation == DEGREE_270) { // 8-270
		rotate = IMAGE_UTIL_ROTATION_270;
	} else {
		rotate = IMAGE_UTIL_ROTATION_NONE;
	}

	dcm_debug("orientation: %d, rotate: %d", orientation, rotate);

	if (rotate == IMAGE_UTIL_ROTATION_90 || rotate == IMAGE_UTIL_ROTATION_270) {
		rotated_width = *buff_height;
		rotated_height = *buff_width;
	} else {
		rotated_width = *buff_width;
		rotated_height = *buff_height;
	}

	/* Calculate the rotated buffer size */
	ret = image_util_calculate_buffer_size(rotated_width, rotated_height, colorspace, &rotated_buffer_size);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Failed to calculate buffer size! err: %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	/* Allocate rotated buffer */
	if (rotated_buffer_size <= 0) {
		dcm_error("Invalid rotated buffer size!");
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	dcm_debug("rotate buffer size: %u", rotated_buffer_size);
	rotated_buffer = (unsigned char *) g_malloc0(rotated_buffer_size);
	if (rotated_buffer == NULL) {
		dcm_error("rotated_buffer is NULL!");
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	try {
		/* Rotate input bitmap */
		ret = image_util_rotate(rotated_buffer, &rotated_width, &rotated_height, rotate, source,
		*buff_width, *buff_height, colorspace);

		if (ret != IMAGE_UTIL_ERROR_NONE || rotated_buffer == NULL) {
			dcm_error("Failed to rotate image buffer! err: %d", ret);
			throw DCM_ERROR_IMAGE_UTIL_FAILED;
		}

		/* Decoded buffer size is set to rotated buffer size to match buffer */
		dcm_debug("After rotation: [width: %d] [height: %d]", rotated_width, rotated_height);
		*buff_width = rotated_width;
		*buff_height = rotated_height;

		/* Allocated pixel should be freed when scanning is finished for this rotated bitmap */
		*image_buffer = rotated_buffer;
		dcm_warn("rotated decode buffer: %p", *image_buffer);

	} catch (DcmErrorType &e) {
		DCM_SAFE_FREE(rotated_buffer);
		dcm_error("image util rotate error!");
		return e;
	}

	return DCM_SUCCESS;
}

int dcm_decode_image(const char *file_path,
	dcm_image_codec_type_e decode_type, const char* mimne_type, bool resize,
	unsigned char **image_buffer, unsigned int *buff_width, unsigned int *buff_height,
	int orientation, unsigned int *size)
{
	int ret = IMAGE_UTIL_ERROR_NONE;
	image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
	unsigned char *decode_buffer = NULL;
	unsigned int decode_width = 0, decode_height = 0;
	unsigned char *resize_buffer = NULL;
	unsigned int resize_buffer_size = 0;
	image_util_decode_h handle = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(file_path, DCM_ERROR_INVALID_PARAMETER);

	ret = image_util_decode_create(&handle);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Error image_util_decode_create ret : %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}	

	if (strcmp(mimne_type, MIME_TYPE_PNG) == 0) {
		DCM_CHECK_VAL((decode_type != DCM_IMAGE_CODEC_RGBA), DCM_ERROR_UNSUPPORTED_IMAGE_TYPE);
	} else if (strcmp(mimne_type, MIME_TYPE_BMP) == 0) {
		DCM_CHECK_VAL((decode_type != DCM_IMAGE_CODEC_RGBA), DCM_ERROR_UNSUPPORTED_IMAGE_TYPE);
	} else if (strcmp(mimne_type, MIME_TYPE_JPEG) == 0) {
		if (decode_type == DCM_IMAGE_CODEC_I420) {
			colorspace = IMAGE_UTIL_COLORSPACE_I420;
		} else if (decode_type == DCM_IMAGE_CODEC_RGB) {
			colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
		} else if (decode_type == DCM_IMAGE_CODEC_RGBA) {
			colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
		} else {
			return DCM_ERROR_UNSUPPORTED_IMAGE_TYPE;
		}
		ret = image_util_decode_set_colorspace(handle, colorspace);
	} else {
		dcm_error("Failed not supported mime type! (%s)", mimne_type);
		return DCM_ERROR_INVALID_PARAMETER;
	}

	ret = image_util_decode_set_input_path(handle, file_path);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Error image_util_decode_set_input_path ret : %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	ret = image_util_decode_set_output_buffer(handle, &decode_buffer);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Error image_util_decode_set_output_buffer ret : %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	ret = image_util_decode_run(handle, (unsigned long *)&decode_width, (unsigned long *)&decode_height, (unsigned long long *)size);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Error image_util_decode_run ret : %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	ret = image_util_decode_destroy(handle);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Error image_util_decode_destroy ret : %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}


	/* Resize the decoded buffer to enhance performance with big size image */
	if (resize) {
		__dcm_decode_calc_image_size(decode_width, decode_height, buff_width, buff_height);
	} else {
		*buff_width = decode_width;
		*buff_height = decode_height;
	}
	if ((decode_width != *buff_width) || (decode_height != *buff_height)) {
		ret = image_util_calculate_buffer_size(*buff_width, *buff_height, colorspace, &resize_buffer_size);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to calculate image buffer size! err: %d", ret);
			return DCM_ERROR_CODEC_DECODE_FAILED;
		}
		resize_buffer = (unsigned char *)malloc(sizeof(unsigned char) * (resize_buffer_size));
		mm_util_resize_image(decode_buffer, decode_width, decode_height, MM_UTIL_IMG_FMT_RGBA8888, resize_buffer, buff_width, buff_height);
	} else {
		resize_buffer = decode_buffer;
	}

	/* Rotate the resized buffer according to orientation */
	if (orientation == 0) {
		*image_buffer = resize_buffer;
	} else {
		ret = _dcm_rotate_image(resize_buffer, image_buffer, buff_width, buff_height, decode_type, orientation);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to rotate image buffer! err: %d", ret);
			return DCM_ERROR_CODEC_DECODE_FAILED;
		}
	}

	dcm_debug_fleave();

	return DCM_SUCCESS;
}
