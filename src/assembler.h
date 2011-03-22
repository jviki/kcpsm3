/**
 * assembler.h
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

#ifndef _PARSER_H
#define _PARSER_H

#include "pico.h"
#include "scanner.h"
#include <stdbool.h>

/**
 * Instruction opcodes for kcpsm3.
 */
enum instr {
	// arithmetics:
	I_ADD_RR 	= 0x19000, I_ADD_RK 	= 0x18000,
	I_ADDCY_RR	= 0x1B000, I_ADDCY_RK	= 0x1A000,
	I_SUB_RR	= 0x1D000, I_SUB_RK 	= 0x1C000,
	I_SUBCY_RR	= 0x1F000, I_SUBCY_RK	= 0x1E000,
	I_AND_RR	= 0x0B000, I_AND_RK 	= 0x0A000,
	I_OR_RR 	= 0x0D000, I_OR_RK  	= 0x0C000,
	I_XOR_RR	= 0x0F000, I_XOR_RK		= 0x0E000,
	I_TEST_RR	= 0x13000, I_TEST_RK	= 0x12000,
	I_COMPARE_RR = 0x15000, I_COMPARE_RK = 0x14000,
	// jumps:
	I_JUMP_COND 	= 0x35000, I_JUMP 	= 0x34000,
	I_CALL_COND 	= 0x31000, I_CALL 	= 0x30000,
	I_RETURN_COND 	= 0x2B000, I_RETURN = 0x2A000,
	// interrupt control:
	I_ENABLE_INTERRUPT = 0x3C001,
	I_DISABLE_INTERRUPT = 0x3C000,
	I_RETURNI_ENABLE = 0x38001,
	I_RETURNI_DISABLE = 0x38000,
	// shifts:
	I_SL0 = 0x20006, I_SL1 = 0x20007,
	I_SLX = 0x20004, I_SLA = 0x20000,
	I_RL  = 0x20002, 
	I_SR0 = 0x2000E, I_SR1 = 0x2000F, 
	I_SRX = 0x2000A, I_SRA = 0x20008, 
	I_RR  = 0x2000C,
	// others:
	I_LOAD_RR	= 0x01000, I_LOAD_RK	= 0x00000,
	I_FETCH_RR	= 0x07000, I_FETCH_RS	= 0x06000,
	I_STORE_RR	= 0x2F000, I_STORE_RS	= 0x2E000,
	I_INPUT_RR	= 0x05000, I_INPUT_RP	= 0x04000,
	I_OUTPUT_RR	= 0x2D000, I_OUTPUT_RP	= 0x2C000
};

#define PADDR_MAX (PROGRAM_LEN-1)
#define SCRATCHPAD_MAX 0x3F

/**
 * Encodes destination register to the instruction
 */
static inline code_t instr_encode_regX(code_t code, int r)
{
	const code_t reg = (code_t) r;
	return (code + (reg << 8));
}

/**
 * Encodes source register to the instruction
 */
static inline code_t instr_encode_regY(code_t code, int r)
{
	const code_t reg = (code_t) r;
	return (code + (reg << 4));
}

/**
 * Encodes constant src value to the instruction
 */
static inline code_t instr_encode_const(code_t code, int c)
{
	return (code + (c & 0x0FF));
}

/**
 * Encodes program address to a jump instruction.
 */
static inline code_t instr_encode_paddr(code_t code, progaddr_t p)
{
	return (code_t) (code + (p & 0x3FF));
}

/**
 * Encodes conditional flag to the instruction.
 */
static inline code_t instr_encode_cond(code_t code, int x)
{
	return (code + (x << 10));
}

/**
 *
 */
bool kcpsm3_setup(struct pico *p);

/**
 * Runs the assembler for the given pico structure.
 */
bool assembler_run(struct pico *pico);

#endif

