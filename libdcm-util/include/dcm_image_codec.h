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

#ifndef _DCM_IMAGE_CODEC_H_
#define _DCM_IMAGE_CODEC_H_

#define DEGREE_0		0
#define DEGREE_90		1
#define DEGREE_180		2
#define DEGREE_270		3

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DCM_IMAGE_FORMAT_I420,
	DCM_IMAGE_FORMAT_RGB,
	DCM_IMAGE_FORMAT_RGBA,
} dcm_image_format_e;

typedef struct {
	int size;
	int width;
	int height;
	int origin_width;
	int origin_height;
	int alpha;
	unsigned char *data;
} dcm_image_info;


int dcm_decode_image(const char *file_path, const dcm_image_format_e format,
	const char* mimne_type, const int orientation, const bool resize,
	unsigned char **image_buffer, unsigned long long *size,
	unsigned int *buff_width, unsigned int *buff_height);

#ifdef __cplusplus
}
#endif

#endif /*_DCM_IMAGE_CODEC_H_*/
