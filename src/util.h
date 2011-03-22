/**
 * util.h
 * Author: Jan Viktorin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static inline char *str_cpy(char *begin, size_t len, char buffer[len + 1])
{
	memcpy(buffer, begin, len);
	buffer[len] = '\0';
	return buffer;
}

static inline unsigned long str2hex(char *s, size_t len)
{
	char *begin = s;
	char *end = s + len;
	unsigned long hex = 0;

	while(begin < end) {
		hex <<= 4;
		if(isdigit(*begin))
			hex += *begin - '0';
		else
			hex += toupper(*begin) + 10 - 'A';

		begin += 1;
	}

	return hex;
}

#endif
