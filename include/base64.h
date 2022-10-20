/*
 * base64.h - base64 conver to bin
 *
 *
 * Copyright (C) 2016-2019, LomboTech Co.Ltd.
 * Author: lomboswer <lomboswer@lombotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef COMMON_INCLUDE_BASE64_H_
#define COMMON_INCLUDE_BASE64_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  encode the data
 * @param  *bindata: the data that needs to be encode
 * @param  *base64: base64 code value
 * @param  binlength: data length
 * @retval base64 code
 */
char *base64_encode(const unsigned char *bindata, int binlength, char *base64);

/**
 * @brief  base64 data the decode
 * @param  *base64:base64 code value
 * @param  *bindata: decode the data
 * @retval 0 if success
 */
int base64_decode(const char *base64, unsigned char *bindata);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  //  COMMON_INCLUDE_BASE64_H_