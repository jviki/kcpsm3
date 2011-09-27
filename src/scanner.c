/**
 * scanner.c
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
#include "scanner.h"
#include "buffer.h"
#include "util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>

static inline bool token_ok(struct pico *p)
{
	assert(p->tok != NULL);
	static struct stab_data *last_data = NULL;

	switch(p->tok->type) {
	case T_END:
		fprintf(stderr, "[%d/%d] Got T_END\n", p->lineno, p->address);
		break;
	case T_LBRACKET:
		fprintf(stderr, "[%d/%d] Got T_LBRACKET\n", p->lineno, p->address);
		break;
	case T_RBRACKET:
		fprintf(stderr, "[%d/%d] Got T_RBRACKET\n", p->lineno, p->address);
		break;
	case T_COMMA:
		fprintf(stderr, "[%d/%d] Got T_COMMA\n", p->lineno, p->address);
		break;
	case T_COLON:
		fprintf(stderr, "[%d/%d] Got T_COLON\n", p->lineno, p->address);
		break;
	case T_NUMBER:
		fprintf(stderr, "[%d/%d] Got T_NUMBER %X\n", p->lineno, p->address, p->tok->value.n);
		break;
	case T_PROGADDR:
		fprintf(stderr, "[%d/%d] Got T_PROGADDR %X\n", p->lineno, p->address, p->tok->value.a);
		break;
	case T_UNKNOWN:
		fprintf(stderr, "[%d/%d] Got T_UNKNOWN\n", p->lineno, p->address);
		break;
	case T_LITERAL:
		{
			struct stab_data *data = p->tok->value.l;
			if(data != last_data)
				fprintf(stderr, "[%d/%d] Got T_LITERAL '%s' (%d)", p->lineno, p->address, data->key, data->len);
			else
				fprintf(stderr, "... ");

			last_data = data;
			if(data->flg != F_UNKNOWN)
				fprintf(stderr, "; is flag (%d)", data->flg);
			if(data->kw != K_UNKNOWN)
				fprintf(stderr, "; is keyword (%d)", data->kw);
			if(data->wait_queue != NULL)
				fprintf(stderr, "; has non-empty waiting queue");
			switch(data->lit) {
			case L_CONSTANT:
				fprintf(stderr, "; is CONSTANT");
				break;
			case L_REGISTER:
				fprintf(stderr, "; is REGISTER s%X", data->value.reg);
				break;
			case L_LABEL:
				fprintf(stderr, "; is LABEL");
				break;
			default:
				break;
			}
			fputc('\n', stderr);
			break;
		}
	}
	return true;
}
#else
#define token_ok(p) true
#endif /* END DEBUG */

// =================================== //
// ----- scanner implementation ------ //
// =================================== //

#define islit(c) (isalnum(c) || (c) == '_')

static bool read_literal(struct pico *p)
{
	debug_here();
	char *begin = p->offset - 1;

	while(!buffer_isend(p, p->offset)) {
		char c = *(p->offset++);

		if(!islit(c)) {
			p->offset -= 1;
			break;
		}
	}

	const size_t len = p->offset - begin + 1; // contains also the ending '\0'
	char buff[len]; // C99 allocation on stack
	struct stab_data *data = stab_insert(p->stab, str_cpy(begin, len - 1, buff));

	if(data == NULL)
		return error(p, "Memory allocation error");

	p->tok->value.l = data;
	p->tok->type = T_LITERAL;
	return token_ok(p);
}

static bool read_num_lit(struct pico *p)
{
	debug_here();
	if(buffer_isend(p, p->offset) || !isxdigit(p->offset[0]))
		// got: [0-3][^0-9A-Fa-f]
		return read_literal(p);

	debug_here();
	if(buffer_isend(p, p->offset + 1) || !islit(p->offset[1])) {
		debug_here();
		p->tok->type = T_NUMBER;
		p->tok->value.n = str2hex(p->offset - 1, 2);
		p->offset += 1;
		return token_ok(p);
	}

	return read_literal(p);
}

static bool read_num_addr_lit(struct pico *p)
{
	debug_here();
	if(buffer_isend(p, p->offset) || !isxdigit(p->offset[0]))
		// got: [0-3][^0-9A-Fa-f]
		return read_literal(p);

	debug_here();
	if(!buffer_isend(p, p->offset + 1)) {
		debug_here();
		if(!isxdigit(p->offset[1]) && islit(p->offset[1]))
			// got: [0-3][0-9A-Fa-f][G-Zg-z_]
			return read_literal(p);

		debug_here();
		if(!islit(p->offset[1])) {
			// got: [0-3][0-9A-Fa-f][^0-9A-Za-z]
			p->tok->type = T_NUMBER;
			p->tok->value.n = str2hex(p->offset - 1, 2);
			p->offset += 1;
			return token_ok(p);
		}
	}
	else {
		// got: [0-3][0-9A-Fa-f]$
		p->tok->type = T_NUMBER;
		p->tok->value.n = str2hex(p->offset - 1, 2);
		p->offset += 1;
		return token_ok(p);
	}

	debug_here();
	if(buffer_isend(p, p->offset + 2) || !islit(p->offset[2])) {
		p->tok->type = T_PROGADDR;
		p->tok->value.n = str2hex(p->offset - 1, 3);
		p->offset += 2;
		return token_ok(p);
	}

	return read_literal(p);
}

static bool read_flg_lit(struct pico *p)
{
	debug_here();
	if(read_literal(p)) {
		if(p->tok->value.l->len == 1) {
			assert(toupper(p->tok->value.l->key[0]) == 'Z');
			p->tok->value.l->flg = F_ZERO;
			return token_ok(p);
		}
		else
			return true;
	}

	return false;
}

static bool read_notflg_lit(struct pico *p)
{
	debug_here();
	if(read_literal(p)) {
		struct stab_data *data = p->tok->value.l;

		if(data->len != 2 || data->flg != F_UNKNOWN)
			return true;
			
		debug_here();
		if(toupper(data->key[1]) == 'C')
			data->flg = F_NOT_CARRY;
		else if(toupper(data->key[1]) == 'Z')
			data->flg = F_NOT_ZERO;

		return token_ok(p);
	}

	return false;
}

static bool read_flg_num_lit(struct pico *p)
{
	debug_here();

	const bool success = read_num_lit(p);
	if(success && p->tok->type == T_LITERAL) {
		struct stab_data *data = p->tok->value.l;

		if(data->len != 1 || data->flg != F_UNKNOWN)
			return true;

		data->flg = F_CARRY;
		return token_ok(p);
	}
	else if(success)
		return true;

	return false;
}

static void skip_comment(struct pico *p)
{
	debug_here();
	while(!buffer_isend(p, p->offset)) {
		char c = *(p->offset++);

		if(c == '\n') {
			p->lineno += 1;
			break;
		}
	}
}

bool scanner_next(struct pico *p)
{
	debug_here();
	assert(p != NULL);
	assert(p->buff != NULL);
	assert(p->tok != NULL);

	while(!buffer_isend(p, p->offset)) {
		char c = *(p->offset++);

		if(isspace(c)) {
			if(c == '\n') {
				p->lineno += 1;
			}

			continue;
		}

		if(toupper(c) == 'C')
			return read_flg_num_lit(p);
		if(toupper(c) == 'Z')
			return read_flg_lit(p);
		if(toupper(c) == 'N')
			return read_notflg_lit(p);
		if(c >= '0' && c <= '3')
			return read_num_addr_lit(p);
		if(isxdigit(c))
			return read_num_lit(p);
		if(islit(c))
			return read_literal(p);

		switch(c) {
		case ',':
			p->tok->type = T_COMMA;
			return token_ok(p);

		case ':':
			p->tok->type = T_COLON;
			return token_ok(p);

		case '(':
			p->tok->type = T_LBRACKET;
			return token_ok(p);

		case ')':
			p->tok->type = T_RBRACKET;
			return token_ok(p);

		case ';':
			skip_comment(p);
			break;

		default:
			return error(p, "Unexpected character in the input");
		}
	}

	p->tok->type = T_END;
	return token_ok(p);
}

// =================================== //
// ------- keyword recognition ------- //
// =================================== //

struct keyword {
	char *name;
	enum keyword_type type;
};

#ifdef SHORTCUTS_EXTENSION
	#define KW_SHORTCUT(s, v) {s, v},
#else
	#define KW_SHORTCUT(s, v)
#endif

static struct keyword keywords[] = {
	{"LOAD", K_LOAD},
	{"FETCH", K_FETCH},
	{"STORE", K_STORE},
	{"INPUT", K_INPUT},
	KW_SHORTCUT("IN", K_INPUT)
	{"OUTPUT", K_OUTPUT},
	KW_SHORTCUT("OUT", K_OUTPUT)
	{"ADD", K_ADD},
	{"ADDCY", K_ADDCY},
	KW_SHORTCUT("ADDC", K_ADDCY)
	{"SUB", K_SUB},
	{"SUBCY", K_SUBCY},
	KW_SHORTCUT("SUBC", K_SUBCY)
	{"AND", K_AND},
	{"OR", K_OR},
	{"XOR", K_XOR},
	{"TEST", K_TEST},
	{"COMPARE", K_COMPARE},
	KW_SHORTCUT("COMP", K_COMPARE)
	{"JUMP", K_JUMP},
	{"CALL", K_CALL},
	{"RETURN", K_RETURN},
	KW_SHORTCUT("RET", K_RETURN)
	{"SR0", K_SR0},
	{"SR1", K_SR1},
	{"SRA", K_SRA},
	{"SRX", K_SRX},
	{"RR", K_RR},
	{"SL0", K_SL0},
	{"SL1", K_SL1},
	{"SLA", K_SLA},
	{"SLX", K_SLX},
	{"RL", K_RL},
	{"ADDRESS", K_ADDRESS},
	{"CONSTANT", K_CONSTANT},
	{"NAMEREG", K_NAMEREG},
	{"INTERRUPT", K_INTERRUPT},
	KW_SHORTCUT("EINT", K_EINT)
	KW_SHORTCUT("DINT", K_DINT)
	{"RETURNI", K_RETURNI},
	KW_SHORTCUT("RETI", K_RETURNI)
	{"ENABLE", K_ENABLE},
	{"DISABLE", K_DISABLE},
};

#define LONGEST_KEYWORD strlen("INTERRUPT")
#define KEYWORDS_LEN (sizeof(keywords)/sizeof(struct keyword))

int getkw(char *s, size_t len)
{
	if(len > LONGEST_KEYWORD) // too long
		return K_UNKNOWN;

	char uppers[len + 1];
	// convert to upper case for case-insensitive compare
	for(int i = 0; *s; s++, i++)
		uppers[i] = toupper(*s);
	uppers[len] = '\0';
	
	for(unsigned i = 0; i < KEYWORDS_LEN; i++) {
		if(!strcmp(keywords[i].name, uppers)) {
			return keywords[i].type;
		}
	}

	return K_UNKNOWN;
}

// =================================== //
// ---- stab_data implementation -----	//
// =================================== //

#define STAB_DATA_LEN sizeof(struct stab_data)
struct stab_data *stab_newdata(char *key)
{
	const int keylen = strlen(key);
	struct stab_data *data = (struct stab_data *) malloc(STAB_DATA_LEN + keylen + 1);

	if(data == NULL)
		return NULL;

	memcpy(data->key, key, keylen + 1);
	data->len = keylen;
	data->flg = F_UNKNOWN;
	data->kw = getkw(key, keylen);
	data->lit = L_UNKNOWN;
	data->wait_queue = NULL;
	return data;
}

void stab_freedata(struct stab_data *data)
{
	free(data);	
}

char *stab_datakey(struct stab_data *data, size_t *keylen)
{
	if(keylen != NULL)
		*keylen = data->len;
	return data->key;
}

int stab_cmpkey(const char *a, const size_t alen, 
						const char *b, const size_t blen)
{
	return strcmp(a, b);
}

