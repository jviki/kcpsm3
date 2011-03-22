/**
 * wait_queue.c
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

#include "pico.h"
#include "wait_queue.h"
#include <stdlib.h>
#include "scanner.h"
#include <stdbool.h>
#include <assert.h>

struct wait_instr {
	code_t code;
	progaddr_t addr;
	enum literal_type expect_type;
	struct wait_instr *next;
};

struct wait_instr *
wait_queue_next(struct wait_instr *instr, progaddr_t *paddr, code_t *code)
{
	assert(instr != NULL);

	*paddr = instr->addr;
	*code = instr->code;
	struct wait_instr *next = instr->next;
	return next;
}

bool wait_queue_append(struct pico *p,
								struct wait_instr **queue, 
								progaddr_t paddr, 
								code_t code, 
								enum literal_type type)
{
	debug_here();
	struct wait_instr *instr = (struct wait_instr *) 
			malloc(sizeof(struct wait_instr));

	if(instr == NULL)
		return error(p, "Memory allocation error");

	if(*queue != NULL && type != (*queue)->expect_type)
		return error(p, "This literal was already used with a different meaning");
	if(type == L_UNKNOWN)
		return error(p, "Fatal error: unexpected unkown literal type");

	instr->addr = paddr;
	instr->code = code;
	instr->expect_type = type;
	instr->next = *queue;
	*queue = instr;
	return true;
}

void wait_queue_remove(struct wait_instr **queue)
{
	struct wait_instr *curr = *queue;
	struct wait_instr *next;

	for(; curr != NULL; curr = next) {
		next = curr->next;
		free(curr);
	}
	*queue = NULL;
}
