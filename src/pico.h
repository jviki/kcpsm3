/**
 * pico.h
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

#ifndef _PICO_H
#define _PICO_H

#include <stdbool.h>
#include <stdlib.h>

struct token;
struct buffer;
struct output;

typedef unsigned int number_t;
typedef unsigned int progaddr_t;
typedef unsigned int code_t;

#define PROGRAM_LEN 1024

struct pico {
	struct token *tok;
	struct buffer *buff;
	struct stab *stab;
	struct output *output;
	char *offset;
	int lineno;
	progaddr_t address;
};

/**
 * @return always false
 */
bool error(struct pico *p, char *msg);

#if DEBUG
	#define debug(s) _debug(s, __FILE__, __LINE__, __func__)
	#define debug_here() _debug(NULL, __FILE__, __LINE__, __func__)

#include <stdio.h>
static inline void _debug(char *s, const char *f, int l, const char *fce)
{
	if(s != NULL)
		fprintf(stderr, "[%s:%d] %s: %s\n", f, l, fce, s);
	else
		fprintf(stderr, "[%s:%d] %s\n", f, l, fce);
}
#else
	#define debug(s)
	#define debug_here()
#endif /* END DEBUG */

#endif

