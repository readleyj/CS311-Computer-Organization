/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

enum instr_type{R, I, J};

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) 
{ 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

enum instr_type get_type(short op_code, enum instr_type type)
{
    switch (op_code) {
        case 0x0:
        {
           type = R;
           break;
        }

        case 0x9:
        case 0xc:
        case 0xf:
        case 0xd:
        case 0xb:
        case 0x23:
        case 0x2b:
        case 0x4:
        case 0x5:
        {
            type = I;
            break;
        }

        case 0x2:
        case 0x3:
        {
            type = J;
            break;
        }
    }

    return type;
}

void make_r(instruction* instr, CPU_State * state)
{
    short funct_code = FUNC(instr);
    unsigned char rs = RS(instr);
    unsigned char rd = RD(instr);
    unsigned char rt = RT(instr);
    unsigned char shamt = SHAMT(instr);

    switch(funct_code)
    {
        case 0x21: // ADDU
        {
            state->REGS[rd] = state->REGS[rs] + state->REGS[rt];
            break;
        }

        case 0x24: // AND
        {
            state->REGS[rd] = state->REGS[rs] & state->REGS[rt];
            break;
        }

        case 0x08: // JR
        {
            state->PC = state->REGS[rs];
            return;
        }

        case 0x27: // NOR
        {
            state->REGS[rd] = ~(state->REGS[rs] | state->REGS[rt]);
            break;
        }

        case 0x25: // OR
        {
            state->REGS[rd] = state->REGS[rs] | state->REGS[rt];
            break;
        }

        case 0x2B: // SLTU
        {
            state->REGS[rd] = state->REGS[rs] < state->REGS[rt];
            break;
        }

        case 0x00: // SLL
        {
            state->REGS[rd] = state->REGS[rt] << shamt;
            break;
        }

        case 0x02: // SRL
        {
            state->REGS[rd] = state->REGS[rt] >> shamt;
            break;
        }

        case 0x23: // SUBU
        {
            state->REGS[rd] = state->REGS[rs] - state->REGS[rt];
            break;
        }
    }

    state->PC += BYTES_PER_WORD;
}

void make_i(instruction* instr, CPU_State * state)
{
    short op_code = OPCODE(instr);
    unsigned char rs = RS(instr);
    unsigned char rt = RT(instr);
    short imm = IMM(instr);

    switch(op_code)
    {
        case 0x09: // ADDIU
        {
            state->REGS[rt] = state->REGS[rs] + imm;
            break;
        }

        case 0x0C: // ANDI
        {
            state->REGS[rt] = state->REGS[rs] & imm;
            break;
        }

        case 0x04: // BEQ
        {
            if (state->REGS[rs] == state->REGS[rt])
                state->PC += (imm * BYTES_PER_WORD);

            break;
        }

        case 0x05: // BNE
        {
            if (state->REGS[rs] != state->REGS[rt])
                state->PC += (imm * BYTES_PER_WORD);

            break;
        }

        case 0x0F: // LUI
        {
            state->REGS[rt] = imm << 16;
            break;
        }

        case 0x23: // LW
        {
            state->REGS[rt] = mem_read_32(state->REGS[rs] + imm);
            break;
        }

        case 0x0D: // ORI
        {
            state->REGS[rt] = state->REGS[rs] | imm;
            break;
        }

        case 0x0B: // SLTIU
        {
            state->REGS[rt] = state->REGS[rs] < imm;
            break;
        }

        case 0x2B: // SW
        {
            mem_write_32(state->REGS[rs] + imm, state->REGS[rt]);
            break;
        }
    }

    state->PC += BYTES_PER_WORD;
}

void make_j(instruction* instr, CPU_State * state)
{
    short op_code = OPCODE(instr);
    uint32_t target = TARGET(instr);

    switch (op_code)
    {
        case 0x2: // J
        {
            state->PC = (target << 2);
            return;
        }
        case 0x3: // JAL
        {
            state->REGS[31] = state->PC + BYTES_PER_WORD;
            state->PC = (target << 2);
            return;
        }
    }

}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/

void process_instruction() {
    enum instr_type type;

    if (CURRENT_STATE.PC >= (MEM_TEXT_START + NUM_INST * BYTES_PER_WORD))
    {
        RUN_BIT = FALSE;
        return;
    }

    instruction* instr = get_inst_info(CURRENT_STATE.PC);
    short op_code = OPCODE(instr);
    type = get_type(op_code, type);

    if (type == R)
    {
        make_r(instr, &CURRENT_STATE);
    }

    else if (type == I)
    {
        make_i(instr, &CURRENT_STATE);
    }

    else if (type == J)
    {
        make_j(instr, &CURRENT_STATE);
    }

}
