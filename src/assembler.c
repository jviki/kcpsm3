/**
 * assembler.c
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
#include "stab.h"
#include "scanner.h"
#include "assembler.h"
#include "output.h"
#include "wait_queue.h"
#include <assert.h>

/**
 * Pushedback token
 */
static struct token *pushback = NULL;

#define PUSHBACK_TOKEN(t) {pushback = t; debug("pushback");};

#define GET_TOKEN(p) {\
	if(pushback != NULL) {\
		p->tok = pushback;\
		pushback = NULL;\
	}\
	else if(!scanner_next(p))\
		return false;}

#define GET_TOKEN_AND_MATCH(p, t, m) {\
	if(pushback != NULL) {\
		p->tok = pushback;\
		pushback = NULL;\
	}\
	else if(!scanner_next(p))\
		return false;\
	if(p->tok->type != t) {\
		return error(p, m);\
	}\
	else if(p->tok->type != t)\
		return error(p, m);\
}

#define MATCH_KEYWORD(p, k, msg) \
	if(p->tok->type != T_LITERAL || p->tok->value.l->kw != k)\
		return error(p, msg);

#define MATCH_LITERAL(p, k, msg) \
	if(p->tok->type != T_LITERAL || p->tok->value.l->lit != k)\
		return error(p, msg);

static bool finish_wait_instr(struct pico *pico, struct stab_data *data)
{
	debug_here();
	struct wait_instr *instr = data->wait_queue;
	if(instr == NULL)
		return true;

	do {
		progaddr_t paddr;
		code_t code;
		instr = wait_queue_next(instr, &paddr, &code);

		switch(data->lit) {
		case L_CONSTANT:
			code = instr_encode_const(code, data->value.n); 
			break;
		case L_REGISTER:
			return error(pico, "Fatal error: register must be set at time");
			break;
		case L_LABEL:
			code = instr_encode_paddr(code, data->value.a);
			break;
		default:
			return error(pico, "Fatal error: can not finish instruction");
		}

		output_code(pico, paddr, code);
	} while(instr != NULL);

	wait_queue_remove(&data->wait_queue);
	return true;
}

/**
 * Implementation of pseudo ADDRESS..
 */
static bool set_address(struct pico *pico)
{
	debug_here();
	GET_TOKEN_AND_MATCH(pico, T_PROGADDR, "Expected PROGRAM ADDRESS here");
	pico->address = pico->tok->value.a;
	return true;
}

/**
 * Implementation of pseudo CONSTANT.
 */
static bool set_constant(struct pico *pico)
{
	debug_here();
	GET_TOKEN_AND_MATCH(pico, T_LITERAL, "Expected a LITERAL here");
	struct stab_data *name = pico->tok->value.l;

	if(name->lit != L_UNKNOWN)
		return error(pico, "A constant or something of this name "
							"already exists");

	GET_TOKEN_AND_MATCH(pico, T_COMMA, "Expected a COMMA here");
	GET_TOKEN_AND_MATCH(pico, T_NUMBER, "Expected a number here");
	const number_t number = pico->tok->value.n;

	name->lit = L_CONSTANT;
	name->value.n = number;
	return finish_wait_instr(pico, name);
}

/**
 * Implementation of pseudo NAMEREG.
 */
static bool set_register(struct pico *pico)
{
	debug_here();
	GET_TOKEN_AND_MATCH(pico, T_LITERAL, "Expected a LITERAL that donates a register");
	MATCH_LITERAL(pico, L_REGISTER, "Expected a register here");
	struct stab_data *regold = pico->tok->value.l;

	GET_TOKEN_AND_MATCH(pico, T_COMMA, "Expected a COMMA here");
	GET_TOKEN_AND_MATCH(pico, T_LITERAL, "Expected a LITERAL here");
	struct stab_data *regnew = pico->tok->value.l;

	if(regnew->lit != L_UNKNOWN)
		return error(pico, "The new name of the register is already in use");

	regnew->lit = L_REGISTER; // create new
	regnew->value.reg = regold->value.reg;
	regold->lit = L_UNKNOWN; // delete old
	return true;
}

static bool ins_interrupt(struct pico *pico, enum instr opcode)
{
	debug_here();
	GET_TOKEN_AND_MATCH(pico, T_LITERAL, "Expected the keyword 'INTERRUPT' here");
	MATCH_KEYWORD(pico, K_INTERRUPT, "Expected the keyword 'INTERRUPT' here");
	return output_code(pico, pico->address++, (code_t) opcode);
}

static bool ins_returni(struct pico *pico)
{
	debug_here();
	GET_TOKEN(pico);

	if(pico->tok->type == T_LITERAL && pico->tok->value.l->kw == K_ENABLE) {
		return output_code(pico, pico->address++, (code_t) I_RETURNI_ENABLE);
	}

	if(pico->tok->type == T_LITERAL && pico->tok->value.l->kw == K_DISABLE) {
		return output_code(pico, pico->address++, (code_t) I_RETURNI_DISABLE);
	}

	return error(pico, "Expected keyword ENABLE or DISABLE here");
}

/**
 * Creates instruction of the given opcode with operands REGISTER, REGISTER.
 */
static code_t assemble_instr_rr(enum instr opcode, int reg1, int reg2)
{
	return instr_encode_regY(instr_encode_regX(opcode, reg1), reg2);
}

/**
 * Creates instruction of the given opcode with operands REGISTER, DIRECT VALUE.
 */
static code_t assemble_instr_rk(enum instr opcode, int reg1, int number)
{
	return instr_encode_const(instr_encode_regX(opcode, reg1), number);
}

static bool ins_arithmetic(struct pico *pico, enum instr rr, enum instr rk)
{
	debug_here();
	GET_TOKEN_AND_MATCH(pico, T_LITERAL, "Expected a LITERAL that donates a register");
	MATCH_LITERAL(pico, L_REGISTER, "Expected a LITERAL that donates a register");
	struct stab_data *dst = pico->tok->value.l;

	GET_TOKEN_AND_MATCH(pico, T_COMMA, "Expected a COMMA here");
	GET_TOKEN(pico);
	debug_here();

	if(pico->tok->type == T_LITERAL) {
		debug_here();
		struct stab_data *src = pico->tok->value.l;

		if(src->lit == L_REGISTER)
			return output_code(pico, pico->address++, 
				assemble_instr_rr(rr, dst->value.reg, src->value.reg));

		if(src->lit == L_CONSTANT) 
			return output_code(pico, pico->address++,
				assemble_instr_rk(rk, dst->value.reg, src->value.n));	
	
		if(src->lit == L_UNKNOWN) {
			enum instr opcode = assemble_instr_rk(rk, dst->value.reg, 0);
			return wait_queue_append(pico, &src->wait_queue, 
					pico->address++, opcode, L_CONSTANT);
		}

		else
			return error(pico, "Unexpected symbol for source");
	}

	if(pico->tok->type == T_NUMBER) {
		debug_here();
		int number = pico->tok->value.n;
		return output_code(pico, pico->address++, 
			assemble_instr_rk(rk, dst->value.reg, number));
	}

	return error(pico, "Unexpected token to specify source of the operation");
}

static bool ins_indirect_access(struct pico *p, enum instr rr, enum instr rk,
		int *specvalue)
{
	debug_here();
	GET_TOKEN_AND_MATCH(p, T_LITERAL, "Expected a LITERAL that donates a register");
	MATCH_LITERAL(p, L_REGISTER, "Expected a register for scratchpad manipulation or I/O");
	struct stab_data *data = p->tok->value.l;

	GET_TOKEN_AND_MATCH(p, T_COMMA, "Expected a COMMA here");
	GET_TOKEN(p);

	if(p->tok->type == T_LBRACKET) {
		GET_TOKEN_AND_MATCH(p, T_LITERAL, "Expected a LITERAL here");
		struct stab_data *spec = p->tok->value.l;
		GET_TOKEN_AND_MATCH(p, T_RBRACKET, "Expected the RIGHT BRACKET here");

		if(spec->lit != L_REGISTER)
			return error(p, "Expected a register to specify scratchpad address or I/O port");

		return output_code(p, p->address++, 
			assemble_instr_rr(rr, data->value.reg, spec->value.reg));
	}
	else if(p->tok->type == T_LITERAL) {
		struct stab_data *spec = p->tok->value.l;
		
		if(spec->lit == L_CONSTANT) {
			const int specval = spec->value.n;
			*specvalue = specval;

			return output_code(p, p->address++, 
				assemble_instr_rk(rk, data->value.reg, specval));
		}
		if(spec->lit == L_UNKNOWN) {
			enum instr opcode = assemble_instr_rk(rk, data->value.reg, 0);
			return wait_queue_append(p, &spec->wait_queue, 
					p->address++, opcode, L_CONSTANT);
		}
#if SHORTCUTS_EXTENSION
		if(spec->lit == L_REGISTER) {
			struct stab_data *spec = p->tok->value.l;
			return output_code(p, p->address++, assemble_instr_rr(rr, data->value.reg, spec->value.reg));
		}
#endif
	}	
	else if(p->tok->type == T_NUMBER) {
		const int spec = p->tok->value.n;
		*specvalue = spec;	

		return output_code(p, p->address++, 
			assemble_instr_rk(rk, data->value.reg, spec));
	}
	
	return error(p, "Unexpected token to specify scratchpad address or I/O port");
}

static bool ins_inout(struct pico *pico, enum instr rr, enum instr rp)
{
	int specval = -1;
	return ins_indirect_access(pico, rr, rp, &specval);
}

static bool ins_scratchpad(struct pico *pico, enum instr rr, enum instr rs)
{
	int specval = -1;
	if(!ins_indirect_access(pico, rr, rs, &specval))
		return false;

	if(specval > SCRATCHPAD_MAX)
		return error(pico, "Invalid scratchpad address, must be in range <0; 3F>");

	return true;
}

static bool ins_jump(struct pico *pico, enum instr icond, enum instr ins)
{
	debug_here();
	enum instr opcode = ins;

	GET_TOKEN(pico);
	if(pico->tok->type == T_LITERAL && pico->tok->value.l->flg != F_UNKNOWN) {
		debug_here();
		const int flag = pico->tok->value.l->flg;
		opcode = instr_encode_cond(icond, flag);

		GET_TOKEN_AND_MATCH(pico, T_COMMA, "Expected a COMMA here");
		GET_TOKEN(pico);
		// loaded token, will be processed next...
	}

	if(pico->tok->type == T_LITERAL) {
		debug_here();
		struct stab_data *paddr = pico->tok->value.l;
		
		if(paddr->lit == L_LABEL) {
			debug_here();
			return output_code(pico, pico->address++,
				instr_encode_paddr(opcode, paddr->value.a));
		}
		else if(paddr->lit != L_UNKNOWN)
			return error(pico, "Illegal target of jump, must be a "
								"label or program address");

		return wait_queue_append(pico, &paddr->wait_queue, 
				pico->address++, opcode, L_LABEL);
		//return output_code(pico, jump->addr, jump->code);
	}
	else if(pico->tok->type == T_PROGADDR) {
		debug_here();
		return output_code(pico, pico->address++,
			instr_encode_paddr(opcode, pico->tok->value.a));
	}

	return error(pico, "Unexpected token, don't know where to jump");
}

static bool ins_return(struct pico *pico, enum instr icond, enum instr ins)
{
	debug_here();
	GET_TOKEN(pico);

	if(pico->tok->type == T_LITERAL && pico->tok->value.l->flg != F_UNKNOWN) {
		int flag = pico->tok->value.l->flg;
		return output_code(pico, pico->address++, instr_encode_cond(icond, flag));
	}
	else
		PUSHBACK_TOKEN(pico->tok);

	return output_code(pico, pico->address++, ins);
}

static bool ins_shift(struct pico *pico, enum instr opcode)
{
	debug_here();
	GET_TOKEN_AND_MATCH(pico, T_LITERAL, "Expected a LITERAL that donates a register");
	MATCH_LITERAL(pico, L_REGISTER, "Unexpected token LITERAL, want to shift a register");
	struct stab_data *reg = pico->tok->value.l;


	return output_code(pico, pico->address++, 
		instr_encode_regX(opcode, reg->value.reg));
}

static bool operation(struct pico *pico, struct token *tok)
{
	debug_here();
	assert(tok->type == T_LITERAL);
	
	const enum keyword_type type = tok->value.l->kw;
	switch(type) {
		case K_ADDRESS:
			return set_address(pico);
		case K_CONSTANT:
			return set_constant(pico);
		case K_NAMEREG:
			return set_register(pico);

		case K_ENABLE:
			return ins_interrupt(pico, I_ENABLE_INTERRUPT);
		case K_DISABLE:
			return ins_interrupt(pico, I_DISABLE_INTERRUPT);

		case K_RETURNI:
			return ins_returni(pico);

		#ifdef SHORTCUTS_EXTENSION
		case K_EINT:
			return output_code(pico, pico->address++, (code_t) I_ENABLE_INTERRUPT);
		case K_DINT:
			return output_code(pico, pico->address++, (code_t) I_DISABLE_INTERRUPT);
		#endif

		case K_INPUT:
			return ins_inout(pico, I_INPUT_RR, I_INPUT_RP);
		case K_OUTPUT:
			return ins_inout(pico, I_OUTPUT_RR, I_OUTPUT_RP);

		case K_LOAD:
			return ins_arithmetic(pico, I_LOAD_RR, I_LOAD_RK);
		case K_ADD:
			return ins_arithmetic(pico, I_ADD_RR, I_ADD_RK);
		case K_ADDCY:
			return ins_arithmetic(pico, I_ADDCY_RR, I_ADDCY_RK);
		case K_SUB:	
			return ins_arithmetic(pico, I_SUB_RR, I_SUB_RK);
		case K_SUBCY:	
			return ins_arithmetic(pico, I_SUBCY_RR, I_SUBCY_RK);
		case K_AND:	
			return ins_arithmetic(pico, I_AND_RR, I_AND_RK);
		case K_OR:
			return ins_arithmetic(pico, I_OR_RR, I_OR_RK);
		case K_XOR:
			return ins_arithmetic(pico, I_XOR_RR, I_XOR_RK);
		case K_TEST:
			return ins_arithmetic(pico, I_TEST_RR, I_TEST_RK);
		case K_COMPARE:
			return ins_arithmetic(pico, I_COMPARE_RR, I_COMPARE_RK);
		
		case K_FETCH:
			return ins_scratchpad(pico, I_FETCH_RR, I_FETCH_RS);
		case K_STORE:	
			return ins_scratchpad(pico, I_STORE_RR, I_STORE_RS);
		
		case K_JUMP:
			return ins_jump(pico, I_JUMP_COND, I_JUMP);
		case K_CALL:
			return ins_jump(pico, I_CALL_COND, I_CALL);
		case K_RETURN:
			return ins_return(pico, I_RETURN_COND, I_RETURN);

		case K_SR0:
			return ins_shift(pico, I_SR0);
		case K_SR1:
			return ins_shift(pico, I_SR1);
		case K_SRA:
			return ins_shift(pico, I_SRA);
		case K_SRX:
			return ins_shift(pico, I_SRX);
		case K_RR:
			return ins_shift(pico, I_RR);

		case K_SL0:
			return ins_shift(pico, I_SL0);
		case K_SL1:
			return ins_shift(pico, I_SL1);
		case K_SLA:
			return ins_shift(pico, I_SLA);
		case K_SLX:
			return ins_shift(pico, I_SLX);
		case K_RL:
			return ins_shift(pico, I_RL);

		default:
			return error(pico, "Unknown keyword detected, maybe an internal error");
	}
}

/**
 * Recognizes a label and stores it into the symbol table.
 */
static bool label(struct pico *pico, struct token *tok)
{
	debug_here();
	assert(tok->type == T_LITERAL);
	assert(tok->value.l != NULL);

	struct stab_data *data = tok->value.l;
	GET_TOKEN_AND_MATCH(pico, T_COLON, "Expected colon after the literal");

	if(data->lit != L_UNKNOWN)
		return error(pico, "The label name was already defined");

	data->lit = L_LABEL;
	data->value.a = pico->address;

	// output the incomplete jumps dependent on this label
	return finish_wait_instr(pico, data);
}

static bool operation_list(struct pico *pico)
{
	debug_here();
	do {
		GET_TOKEN(pico);
		struct token *tok = pico->tok;

		if(tok->type == T_LITERAL && tok->value.l->kw == K_UNKNOWN) {
			debug_here();
			if(!label(pico, tok))
				return false;
		}
		else if(tok->type == T_LITERAL) {
			debug_here();
			if(!operation(pico, tok))
				return false;
		}
		else if(tok->type != T_END)	
			return error(pico, "Unexpected token detected");

	} while(pico->tok->type != T_END);

	return true;
}

bool assembler_run(struct pico *pico)
{
	struct token tok;

	pico->tok = &tok;
	return operation_list(pico);
}

bool kcpsm3_setup(struct pico *pico)
{
	static char *regs[] = {
			"s0", "s1", "s2", "s3",
			"s4", "s5", "s6", "s7",
			"s8", "s9", "sA", "sB",
			"sC", "sD", "sE", "sF"
		};
	#define REG_COUNT (sizeof(regs)/sizeof(char *))
	struct stab_data *data = NULL;

	for(unsigned i = 0; i < REG_COUNT; i++) {
		data = stab_insert(pico->stab, regs[i]);
		if(data == NULL)
			return false;

		data->lit = L_REGISTER;
		data->value.reg = i;
	}

	return true;
}

