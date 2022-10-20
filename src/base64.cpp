/*
 * base64.c - base64 conversion to bin
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

#include "base64.h"

#include <string.h>
const char *base64char =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/*3-4*/

/**
 * @brief  encode the data
 * @param  *bindata: the data that needs to be encode
 * @param  *base64: base64 code value
 * @param  binlength: data length
 * @retval base64 code
 */
char *base64_encode(const unsigned char *bindata, int binlength, char *base64)
{
	int i, j;
	unsigned char current;

	for (i = 0, j = 0; i < binlength; i += 3) {
		current = (bindata[i] >> 2);
		current &= (unsigned char) 0x3F;
		base64[j++] = base64char[(int) current];

		current = ((unsigned char)(bindata[i] << 4)) & ((unsigned char) 0x30);
		if (i + 1 >= binlength) {
			base64[j++] = base64char[(int) current];
			base64[j++] = '=';
			base64[j++] = '=';
			break;
		}
		current |= ((unsigned char)(bindata[i + 1] >> 4))
			& ((unsigned char) 0x0F);
		base64[j++] = base64char[(int) current];

		current = ((unsigned char)(bindata[i + 1] << 2))
			& ((unsigned char) 0x3C);
		if (i + 2 >= binlength) {
			base64[j++] = base64char[(int) current];
			base64[j++] = '=';
			break;
		}
		current |= ((unsigned char)(bindata[i + 2] >> 6))
			& ((unsigned char) 0x03);
		base64[j++] = base64char[(int) current];

		current = ((unsigned char) bindata[i + 2]) & ((unsigned char) 0x3F);
		base64[j++] = base64char[(int) current];
	}
	base64[j] = '\0';
	return base64;
}
/*4-3*/
/**
 * @brief  base64 data the decode
 * @param  *base64:base64 code value
 * @param  *bindata: decode the data
 * @retval 0 if success
 */
int base64_decode(const char *base64, unsigned char *bindata)
{

	int i, j;
	unsigned char k;
	unsigned char temp[4];
	for (i = 0, j = 0; base64[i] != '\0'; i += 4) {
		memset(temp, 0xFF, sizeof(temp));
		for (k = 0; k < 64; k++) {
			if (base64char[k] == base64[i])
				temp[0] = k;
		}
		for (k = 0; k < 64; k++) {
			if (base64char[k] == base64[i + 1])
				temp[1] = k;
		}
		for (k = 0; k < 64; k++) {
			if (base64char[k] == base64[i + 2])
				temp[2] = k;
		}
		for (k = 0; k < 64; k++) {
			if (base64char[k] == base64[i + 3])
				temp[3] = k;
		}

		bindata[j++] =
			((unsigned char)(((unsigned char)(temp[0] << 2)) & 0xFC))
			| ((unsigned char)((unsigned char)(temp[1] >> 4)
					& 0x03));
		if (base64[i + 2] == '=')
			break;

		bindata[j++] =
			((unsigned char)(((unsigned char)(temp[1] << 4)) & 0xF0))
			| ((unsigned char)((unsigned char)(temp[2] >> 2)
					& 0x0F));
		if (base64[i + 3] == '=')
			break;

		bindata[j++] =
			((unsigned char)(((unsigned char)(temp[2] << 6)) & 0xF0))
			| ((unsigned char)(temp[3] & 0x3F));
	}
	return j;
}
