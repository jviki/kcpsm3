/**
 * scanner.h
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

#ifndef _SCANNER_H
#define _SCANNER_H

#include "pico.h"
#include "stab.h"
#include <stdbool.h>

enum token_type {
	T_UNKNOWN = -1,
	T_LITERAL,
	T_NUMBER,
	T_PROGADDR,
	T_COMMA,
	T_COLON,
	T_LBRACKET,
	T_RBRACKET,
	T_END
};

enum flag {
	F_UNKNOWN = -1,
	F_CARRY = 2, F_NOT_CARRY = 3,
	F_ZERO = 0, F_NOT_ZERO = 1
};

enum keyword_type {
	K_UNKNOWN = -1,
	K_LOAD,
	K_FETCH,	K_STORE,
	K_INPUT, K_OUTPUT,
	K_ADD,	K_ADDCY,
	K_SUB,	K_SUBCY,
	K_AND,	K_OR,
	K_XOR,	K_TEST,
	K_COMPARE,
	K_JUMP,	K_CALL,
	K_RETURN,
	K_RETURNI,	K_INTERRUPT,
	K_ENABLE,	K_DISABLE,
	#ifdef SHORTCUTS_EXTENSION
	K_EINT, K_DINT,
	#endif
	K_SR0,	K_SR1,
	K_SRA,	K_SRX,
	K_RR,
	K_SL0,	K_SL1,
	K_SLA,	K_SLX,
	K_RL,
	K_ADDRESS,
	K_CONSTANT,
	K_NAMEREG
};

union token_value {
	struct stab_data *l;
	enum flag f;
	number_t n;
	progaddr_t a;
};

struct token {
	enum token_type type;
	union token_value value;
	int lineno;
};

enum literal_type {
	L_UNKNOWN, L_REGISTER, L_CONSTANT, L_LABEL 
};

union literal_value {
	int reg;
	progaddr_t a;
	number_t n;
};

/**
 *
 */
struct wait_instr;

struct stab_data {
	// table to store incomplete instructions
	struct wait_instr *wait_queue;
	enum literal_type lit;
	union literal_value value;
	enum keyword_type kw;
	enum flag flg;
	size_t len;
	char key[];
};

bool scanner_next(struct pico *p);

#endif
