/**
 * output.c
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

#include "pico.h"
#include "output.h"
#include "assembler.h"
#include "stab.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct output {
	FILE *file;
	code_t code[PROGRAM_LEN];
};

bool output_init(struct pico *p, char *filename)
{
	FILE *out = stdout;

	if(filename != NULL) {
		out = fopen(filename, "w");
		if(out == NULL) 
			return error(NULL, "Can not open the output file");
	}

	struct output *ins = (struct output *) calloc(1, sizeof(struct output));
	if(ins == NULL) {
		fclose(out);
		return error(NULL, "Memory allocation error");
	}

	ins->file = out;
	p->output = ins;
	return true;
}

void output_destroy(struct pico *p)
{
	if(p->output == NULL)
		return;

	if(p->output->file != NULL)
		fclose(p->output->file);

	free(p->output);
	p->output = NULL;
}

bool output_code(struct pico *p, progaddr_t address, code_t ins)
{
	debug_here();
	if(address >= PROGRAM_LEN)
		return error(p, "Unexpected program address, max 1023");

	p->output->code[address] = ins;
	return true;
}

void output_flush(struct pico *p)
{
	debug_here();
	for(int i = 0; i < PROGRAM_LEN; i++) {
		fprintf(p->output->file, "%.5X\n", p->output->code[i]);
	}
}


