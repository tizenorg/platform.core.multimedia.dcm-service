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

#ifndef _DCM_DEBUG_UTILS_H_
#define _DCM_DEBUG_UTILS_H_

#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define _USE_DLOG_
#define LOG_TAG "DCM_SVC"

/* anci c color type */
#define FONT_COLOR_RESET    "\033[0m"
#define FONT_COLOR_RED      "\033[31m"
#define FONT_COLOR_GREEN    "\033[32m"
#define FONT_COLOR_YELLOW   "\033[33m"
#define FONT_COLOR_BLUE     "\033[34m"
#define FONT_COLOR_PURPLE   "\033[35m"
#define FONT_COLOR_CYAN     "\033[36m"
#define FONT_COLOR_GRAY     "\033[37m"

/*#undef _USE_DLOG_ */
#ifdef _USE_DLOG_
#define dcm_debug(fmt, arg...) do { \
		LOGD(FONT_COLOR_CYAN""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_info(fmt, arg...) do { \
		LOGI(FONT_COLOR_YELLOW""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_warn(fmt, arg...) do { \
		LOGW(FONT_COLOR_GREEN""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_error(fmt, arg...) do { \
		LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_sec_debug(fmt, arg...) do { \
		SECURE_LOGD(FONT_COLOR_CYAN""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_sec_info(fmt, arg...) do { \
		SECURE_LOGI(FONT_COLOR_YELLOW""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_sec_warn(fmt, arg...) do { \
		SECURE_LOGW(FONT_COLOR_GREEN""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)

#define dcm_sec_error(fmt, arg...) do { \
		SECURE_LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg); \
	} while (0)
#define dcm_debug_fenter() do { \
			LOGD(FONT_COLOR_RESET"<Enter>"); \
		} while (0)

#define dcm_debug_fleave() do { \
			LOGD(FONT_COLOR_RESET"<Leave>"); \
		} while (0)
#define dcm_retm_if(expr, fmt, arg...) do { \
			if(expr) { \
				LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg); \
				return; \
			} \
		} while (0)

#define dcm_retvm_if(expr, val, fmt, arg...) do { \
			if(expr) { \
				LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg); \
				return (val); \
			} \
		} while (0)
#else
#define dcm_debug(fmt, arg...)
#define dcm_info(fmt, arg...)
#define dcm_warn(fmt, arg...)
#define dcm_error(fmt, arg...)

#define dcm_sec_debug(fmt, arg...)
#define dcm_sec_info(fmt, arg...)
#define dcm_sec_warn(fmt, arg...)
#define dcm_sec_error(fmt, arg...)

#endif

#define DCM_CHECK_VAL(expr, val) 		dcm_retvm_if(!(expr), val,   "Invalid parameter, return ERROR code!")
#define DCM_CHECK_NULL(expr) 			dcm_retvm_if(!(expr), NULL,  "Invalid parameter, return NULL!")
#define DCM_CHECK_FALSE(expr) 			dcm_retvm_if(!(expr), FALSE, "Invalid parameter, return FALSE!")
#define DCM_CHECK(expr) 				dcm_retm_if (!(expr),        "Invalid parameter, return!")

#define DCM_SAFE_FREE(ptr) { if(ptr) {free(ptr); ptr = NULL;} }

#endif /* _DCM_DEBUG_UTILS_H_ */

