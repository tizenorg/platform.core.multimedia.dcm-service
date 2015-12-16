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

#if 0
int __dcm_decode_image_with_evas(const char *origin_path,
					int dest_width, int dest_height,
					dcm_image_info *image_info)
{
	dcm_debug_fenter();

	Ecore_Evas *resize_img_ee;

	resize_img_ee = ecore_evas_buffer_new(dest_width, dest_height);
	if (!resize_img_ee) {
		dcm_error("ecore_evas_buffer_new failed");
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}

	Evas *resize_img_e = ecore_evas_get(resize_img_ee);
	if (!resize_img_e) {
		dcm_error("ecore_evas_get failed");
		ecore_evas_free(resize_img_ee);
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}

	Evas_Object *source_img = evas_object_image_add(resize_img_e);
	if (!source_img) {
		dcm_error("evas_object_image_add failed");
		ecore_evas_free(resize_img_ee);
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}

	evas_object_image_file_set(source_img, origin_path, NULL);

	/* Get w/h of original image */
	int width = 0;
	int height = 0;

	evas_object_image_size_get(source_img, &width, &height);
	image_info->origin_width = width;
	image_info->origin_height = height;

	evas_object_image_load_orientation_set(source_img, 1);

	ecore_evas_resize(resize_img_ee, dest_width, dest_height);

	evas_object_image_load_size_set(source_img, dest_width, dest_height);
	evas_object_image_fill_set(source_img, 0, 0, dest_width, dest_height);
	evas_object_image_filled_set(source_img, 1);

	evas_object_resize(source_img, dest_width, dest_height);
	evas_object_show(source_img);

	/* Set alpha from original */
	image_info->alpha = evas_object_image_alpha_get(source_img);
	if (image_info->alpha) ecore_evas_alpha_set(resize_img_ee, EINA_TRUE);

	/* Create target buffer and copy origin resized img to it */
	Ecore_Evas *target_ee = ecore_evas_buffer_new(dest_width, dest_height);
	if (!target_ee) {
		dcm_error("ecore_evas_buffer_new failed");
		ecore_evas_free(resize_img_ee);
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}

	Evas *target_evas = ecore_evas_get(target_ee);
	if (!target_evas) {
		dcm_error("ecore_evas_get failed");
		ecore_evas_free(resize_img_ee);
		ecore_evas_free(target_ee);
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}

	Evas_Object *ret_image = evas_object_image_add(target_evas);
	evas_object_image_size_set(ret_image, dest_width, dest_height);
	evas_object_image_fill_set(ret_image, 0, 0, dest_width, dest_height);
	evas_object_image_filled_set(ret_image,	EINA_TRUE);

	evas_object_image_data_set(ret_image, (int *)ecore_evas_buffer_pixels_get(resize_img_ee));
	evas_object_image_data_update_add(ret_image, 0, 0, dest_width, dest_height);

	unsigned int buf_size = 0;
	int stride = evas_object_image_stride_get(ret_image);
	buf_size = (sizeof(unsigned char) * stride * dest_height);
	dcm_debug("buf_size: %d", buf_size);

	image_info->size = buf_size;
	image_info->width = dest_width;
	image_info->height = dest_height;
	image_info->data = (unsigned char *)malloc(buf_size);
	if (image_info->data == NULL) {
		dcm_error("Failed to allocate memory" );
		ecore_evas_free(resize_img_ee);
		ecore_evas_free(target_ee);

		return DCM_ERROR_OUT_OF_MEMORY;
	}

	void *image_data = evas_object_image_data_get(ret_image, 1);
	if (image_data != NULL) {
		memcpy(image_info->data, image_data, buf_size);
	} else {
		dcm_error("image_data is NULL. evas_object_image_data_get failed");
	}

	ecore_evas_free(target_ee);
	ecore_evas_free(resize_img_ee);

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

static int __dcm_codec_get_image_orientation(const char *path, int *orientation)
{
	int ret = DCM_SUCCESS;
	ExifData *ed = NULL;
	ExifEntry *entry = NULL;
	int status = 0;
	ExifByteOrder byte_order;

	DCM_CHECK_VAL(path, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(orientation, DCM_ERROR_INVALID_PARAMETER);

	ed = exif_data_new_from_file(path);
	if (ed == NULL) {
		dcm_error("Failed to create new exif data!");
		ret = DCM_ERROR_EXIF_FAILED;
		goto DCM_SVC_DB_GET_FACE_ORIENTATION_FAILED;
	}

	entry = exif_data_get_entry(ed, EXIF_TAG_ORIENTATION);
	if (entry == NULL) {
		dcm_warn("Exif entry not exist!");
		*orientation = 0;
		ret = DCM_SUCCESS;
		goto DCM_SVC_DB_GET_FACE_ORIENTATION_FAILED;
	}

	byte_order = exif_data_get_byte_order(ed);
	status = exif_get_short(entry->data, byte_order);
	switch (status) {
		case 1:
		case 2:
			*orientation = DEGREE_0;
			break;
		case 3:
		case 4:
			*orientation = DEGREE_180;
			break;
		case 5:
		case 8:
			*orientation = DEGREE_270;
			break;
		case 6:
		case 7:
			*orientation = DEGREE_90;
			break;
		default:
			*orientation = DEGREE_0;
			break;
	}

	dcm_debug("orientation: %d", *orientation);

DCM_SVC_DB_GET_FACE_ORIENTATION_FAILED:

	if (ed != NULL) {
		exif_data_unref(ed);
		ed = NULL;
	}

	return ret;
}
#endif

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

#if 0
static int __dcm_decode_image_with_size_orient(const char *file_path, unsigned int target_width, unsigned int target_height,
	dcm_image_codec_type_e decode_type, unsigned char **image_buffer, unsigned int *buff_width, unsigned int *buff_height, int *orientation, unsigned int *size)
{
	int ret = IMAGE_UTIL_ERROR_NONE;
	image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_I420;
	unsigned char *decode_buffer = NULL;

	dcm_debug_fenter();

	DCM_CHECK_VAL(file_path, DCM_ERROR_INVALID_PARAMETER);

	if (decode_type == DCM_IMAGE_CODEC_I420) {
		colorspace = IMAGE_UTIL_COLORSPACE_I420;
	} else if (decode_type == DCM_IMAGE_CODEC_RGB) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
	} else if (decode_type == DCM_IMAGE_CODEC_RGBA) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
	} else {
		return DCM_ERROR_UNSUPPORTED_IMAGE_TYPE;
	}

	/* Extract orientation from exif */
	ret = __dcm_codec_get_image_orientation(file_path, orientation);
	if (ret != DCM_SUCCESS) {
		dcm_error("Failed to extract orientation! err: %d", ret);
		dcm_warn("Set orientation to default!");
		*orientation = 0;
	}

	ret = image_util_decode_jpeg(file_path, colorspace, &decode_buffer, (int *)buff_width, (int *)buff_height, size);
	dcm_debug("file:%s buffer:%p, size:%u, buff_width:%d, buff_height:%d", file_path, image_buffer, *size, *buff_width, *buff_height);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("Error image_util_decode_jpeg ret : %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	/* Rotate the decoded buffer according to orientation */
	if (*orientation == 0) {
		*image_buffer = decode_buffer;
	} else {
		ret = _dcm_rotate_image(decode_buffer, image_buffer, buff_width, buff_height, decode_type, *orientation);
		if (ret != DCM_SUCCESS) {
			dcm_error("Failed to rotate image buffer! err: %d", ret);
			return DCM_ERROR_CODEC_DECODE_FAILED;
		}
	}

	dcm_debug_fleave();

	return DCM_SUCCESS;
}

int dcm_decode_image_with_size_orient(const char *file_path, unsigned int target_width, unsigned int target_height,
	dcm_image_codec_type_e decode_type, unsigned char **image_buffer, unsigned int *buff_width, unsigned int *buff_height, int *orientation, unsigned int *size)
{
	int ret = DCM_SUCCESS;

	ret = __dcm_decode_image_with_size_orient(file_path, target_width, target_height, decode_type, image_buffer, buff_width, buff_height, orientation, size);
	if (ret != DCM_SUCCESS) {
		dcm_error("Error _dcm_decode_image_with_size ret : %d", ret);
	}

	return ret;
}

int dcm_decode_image_with_evas(const char *file_path, unsigned int target_width, unsigned int target_height,
	dcm_image_codec_type_e decode_type, unsigned char **image_buffer, unsigned int *buff_width, unsigned int *buff_height, int *orientation, unsigned int *size)
{
	int ret = DCM_SUCCESS;
	dcm_image_info image_info = {0, };
	image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_I420;
	mm_util_img_format mm_colorspace = MM_UTIL_IMG_FMT_I420;

	DCM_CHECK_VAL(file_path, DCM_ERROR_INVALID_PARAMETER);

	memset(&image_info, 0, sizeof(image_info));

	if (decode_type == DCM_IMAGE_CODEC_I420) {
		colorspace = IMAGE_UTIL_COLORSPACE_I420;
		mm_colorspace = MM_UTIL_IMG_FMT_I420;
	} else if (decode_type == DCM_IMAGE_CODEC_RGB) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
		mm_colorspace = MM_UTIL_IMG_FMT_RGB888;
	}
	ret = __dcm_decode_image_with_evas(file_path, target_width, target_height, &image_info);
	if (ret != DCM_SUCCESS) {
		dcm_error("Error __dcm_decode_image_with_evas ret : %d", ret);
		*buff_width = 0;
		*buff_height = 0;
		*orientation = 0;
		*size = 0;
		*image_buffer = NULL;
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}

	*buff_width = image_info.width;
	*buff_height = image_info.height;
	*orientation = 0;
	image_util_calculate_buffer_size(*buff_width, *buff_height, colorspace, size);
	*image_buffer = (unsigned char *)malloc(sizeof(unsigned char) * (*size));
	if (*image_buffer == NULL) {
		dcm_error("Error buffer allocation");
		DCM_SAFE_FREE(image_info.data);
		return DCM_ERROR_CODEC_DECODE_FAILED;
	}
	int err = 0;
	err = mm_util_convert_colorspace(image_info.data,
					image_info.width,
					image_info.height,
					MM_UTIL_IMG_FMT_BGRA8888,
					*image_buffer,
					mm_colorspace);

	if (err < 0) {
		dcm_error("Failed to change from argb888 to %d. (%d)", mm_colorspace, err);
		DCM_SAFE_FREE(*image_buffer);
		*image_buffer = NULL;
		ret = DCM_ERROR_CODEC_DECODE_FAILED;
	}
	DCM_SAFE_FREE(image_info.data);

	return ret;
}
#endif
