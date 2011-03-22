/**
 * wait_queue.h
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

#ifndef _WAIT_INSTR_H
#define _WAIT_INSTR_H

#include "pico.h"
#include "scanner.h"
#include <stdbool.h>

struct wait_instr;

bool wait_queue_append(struct pico *p,
								struct wait_instr **queue, 
								progaddr_t paddr, 
								code_t code, 
								enum literal_type type);

/**
 * Each call returns values of the given waiting instruction.
 * Last valid call returns NULL (the values are valid).
 */
struct wait_instr *
wait_queue_next(struct wait_instr *instr, progaddr_t *paddr, code_t *code);

void wait_queue_remove(struct wait_instr **queue);

#endif
