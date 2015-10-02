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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <glib.h>
#include <image_util.h>
#include <dcm_image_codec.h>
#include <DcmDbUtils.h>
#include <DcmTypes.h>
#include <DcmDebugUtils.h>
#include <DcmColorUtils.h>


int DcmColorUtils::runColorExtractProcess(DcmScanItem *scan_item, DcmImageInfo *image_info)
{
	DCM_CHECK_VAL(scan_item, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(image_info, DCM_ERROR_INVALID_PARAMETER);
	DCM_CHECK_VAL(image_info->pixel, DCM_ERROR_INVALID_PARAMETER);

	dcm_debug_fenter();
#if 0
	DcmDbUtils *dcmDbUtils = DcmDbUtils::getInstance();
	int ret = IMAGE_UTIL_ERROR_NONE;
	DcmColorItem colorItem = {0,};
	memset(&colorItem, 0, sizeof(DcmColorItem));

	// Extracting color supports only RGB888 format
	ret = image_util_extract_color_from_memory(image_info->pixel, image_info->buffer_width, image_info->buffer_height, &(colorItem.rgb_r), &(colorItem.rgb_g), &(colorItem.rgb_b));
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		dcm_error("image_util_extract_color_from_memory err= %d", ret);
		return DCM_ERROR_IMAGE_UTIL_FAILED;
	}

	dcm_debug("image_util_extract_color_from_memory result r:%02x, g:%02x, b:%02x", colorItem.rgb_r, colorItem.rgb_g, colorItem.rgb_b);

	colorItem.media_uuid = strdup(scan_item->media_uuid);
	colorItem.storage_uuid = strdup(scan_item->storage_uuid);
	ret = dcmDbUtils->_dcm_svc_db_update_color_to_db(colorItem);
	if (ret != DCM_SUCCESS) {
		dcm_error("Failed to update color item into db! err: %d", ret);
		return DCM_ERROR_DB_OPERATION;
	}
#endif
	dcm_debug_fleave();

	return DCM_SUCCESS;
}

