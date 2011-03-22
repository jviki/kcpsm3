/**
 * output.h
 * Author: Jan Viktorin <xvikto03 (at) stud.fit.vutbr.cz>
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

#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "pico.h"
#include <stdbool.h>

/**
 * Destroys the instance of output structure.
 */
void output_destroy(struct pico *p);

/**
 * Outputs the assembled instruction.
 */
bool output_code(struct pico *p, progaddr_t address, code_t ins);

/**
 * Flushs buffer if any.
 */
void output_flush(struct pico *p);

#endif

