/**
 * main.c
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

#include "pc.h"
#include "buffer.h"
#include "scanner.h"
#include "stab.h"
#include "output.h"
#include "assembler.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#define PROGRAM "Pico Assembler"
#define AUTHOR "Jan Viktorin"
#define YEAR "2010"
#define VERSION "0.1"
#define USAGE "[-i<srcfile>] [-o<hexfile>] [-l<listing>] [-qh]"

static void help(char *pname)
{
	printf("Program '%s' v%s, Copyright (c) %s %s\n", PROGRAM, VERSION, YEAR, AUTHOR);
	#ifdef SHORTCUTS_EXTENSION
	printf("*Compiled with SHORTCUTS_EXTENSION\n");
	#endif
	printf("Usage: %s %s\n", pname, USAGE);
	printf(	"\t-i<srcfile> Source file\n"
				"\t-o<hexfile> Output file, contains instructions in HEX form\n"
				"\t-l<listing> Listing file\n"
				"\t-q          Quite mode, no output messages\n"
				"\t-h          Prints this help\n");
	printf("This program is under GNU GPL license, please see www.gnu.org\n");
}

bool error(struct pico *p, char *msg)
{
	fprintf(stderr, "[l.%d] %s\n", p->lineno, msg);
	return false;
}

static void stab_listing_visit(struct stab_data *data, void *op)
{
	FILE *f = (FILE *) op;
	switch(data->lit) {
	case L_CONSTANT:
		fprintf(f, "%.2X \t%s\n", data->value.n, data->key);
		break;
	case L_LABEL:
		fprintf(f, "%.3X\t%s\n", data->value.a, data->key);
		break;
	default:
		break;
	}
}

static void stab_listing(struct pico *p, char *listing)
{
	if(listing == NULL)
		return;

	FILE *f = fopen(listing, "w");
	fprintf(f, "# Machine generated listing of compilation\n");
	fprintf(f, "# Format: <value> <tab> <name>\n");
	fprintf(f, "# <value> is hexadecimal number (2 digits = CONSTANT, 3 digits = ADDRESS)\n");
	stab_visit(p->stab, &stab_listing_visit, (void *) f);
	fclose(f);
}

int main(int argc, char *argv[argc])
{
	char *srcfile = NULL;
	char *dstfile = NULL;
	char *listing = NULL;
	
	opterr = 0;
	int opt;
	while((opt = getopt(argc, argv, "qhi:o:l:")) != -1) {
		switch(opt) {
		case 'q':
			fclose(stderr);
			break;
		case 'h':
			help(argv[0]);
			return EXIT_SUCCESS;
		case 'i':
			srcfile = optarg;
			break;
		case 'o':
			dstfile = optarg;
			break;
		case 'l':
			listing = optarg;
			break;
		case '?':
			return EXIT_FAILURE;
		}
	}

	struct token tok = {.type = T_UNKNOWN, .lineno = -1};
	struct pico p = {.tok = &tok, .stab = NULL, .address = 0,
		.buff = NULL, .offset = NULL, .lineno = 1};

	p.stab = stab_init();
	if(p.stab == NULL)
		return EXIT_FAILURE;
	
	if(!kcpsm3_setup(&p)) {
		stab_destroy(p.stab);
		return EXIT_FAILURE;
	}

	if(!buffer_init(&p, srcfile)) {
		stab_destroy(p.stab);
		return EXIT_FAILURE;
	}

	if(!output_init(&p, dstfile)) {
		stab_destroy(p.stab);
		buffer_destroy(&p);
		return EXIT_FAILURE;
	}

	int result = EXIT_FAILURE;
	if(assembler_run(&p)) {
		result = EXIT_SUCCESS;
		output_flush(&p);
	}

	buffer_destroy(&p);
	stab_listing(&p, listing);
	stab_destroy(p.stab);
	output_destroy(&p);
	return result;
}

