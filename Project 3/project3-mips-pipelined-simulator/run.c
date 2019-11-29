/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <stdbool.h>

#include "util.h"
#include "run.h"

enum instr_type {R, I, J};

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

void fill_control(ID_EX_Pipe *pipe, const uint16_t signals[NUM_SIGNALS])
{
    EX_control *ex = &pipe->ex_control;
    MEM_control *mem = &pipe->mem_control;
    WB_control *wb = &pipe->wb_control;

    ex->RegDst = (unsigned char) signals[0];
    ex->ALUSrc = (unsigned char) signals[1];
    ex->ALUop0 = (unsigned char) signals[2];
    ex->ALUop1 = (unsigned char) signals[3];
    ex->ALUCntrl = signals[4];
    mem->Branch = (unsigned char) signals[5];
    mem->BranchNE = (unsigned char) signals[6];
    mem->MemRead = (unsigned char) signals[7];
    mem->MemWrite = (unsigned char) signals[8];
    wb->MemtoReg = (unsigned char) signals[9];
    wb->RegWrite = (unsigned char) signals[10];
}

void flush_IF_ID(bool flush_on_next)
{
    CURRENT_STATE.IF_ID_PIPE = (const IF_ID_Pipe) {0};
    CURRENT_STATE.PAST_IF_ID_PIPE = (const IF_ID_Pipe) {0};
    if(!flush_on_next) CURRENT_STATE.PIPE[IF_STAGE] = 0;
}

void flush_ID_EX(bool flush_on_next)
{
    CURRENT_STATE.ID_EX_PIPE = (const ID_EX_Pipe) {0};
    CURRENT_STATE.PAST_ID_EX_PIPE = (const ID_EX_Pipe) {0};
    if (!flush_on_next) CURRENT_STATE.PIPE[ID_STAGE] = 0;
}

void flush_EX_MEM()
{
    CURRENT_STATE.EX_MEM_PIPE = (const EX_MEM_Pipe) {0};
    CURRENT_STATE.PIPE[EX_STAGE] = 0;
}

void encode_control_signals(instruction* instr)
{
    ID_EX_Pipe* pipe = &CURRENT_STATE.ID_EX_PIPE;
    short op_code = OPCODE(instr);
    enum instr_type type;
    type = get_type(op_code, type);

    if (type == R)
    {
        uint16_t ALU_control = 0;
        unsigned char reg_write = 1;
        short func = FUNC(instr);

        switch(func)
        {
            // ADD, ADDU
            case 0x20:
            case 0x21:
                ALU_control = 2;
                break;

            // AND
            case 0x24:
                ALU_control = 0;
                break;

            // SUB, SUBU
            case 0x22:
            case 0x23:
                ALU_control = 6;
                break;

            // OR
            case 0x25:
                ALU_control = 1;
                break;

            // SLT
            case 0x2a:
            case 0x2b:
                ALU_control = 7;
                break;

            // JR
            case 0x8:
                reg_write = 0;
                CURRENT_STATE.PC = CURRENT_STATE.REGS[RS(instr)];
                CURRENT_STATE.JUMP_FLUSH = true;
                break;

            // NOR
            case 0x27:
                ALU_control = 12;
                break;

            // SLL
            case 0x0:
                ALU_control = 3;
                break;

            // SRL
            case 0x2:
                ALU_control = 4;
                break;
        }

        uint16_t signals[NUM_SIGNALS] = {1, 0, 0, 1, ALU_control, 0, 0, 0, 0, 0, reg_write};
        fill_control(pipe, signals);
    }
    else if (type == I)
    {
        switch(op_code)
        {
            // SW
            case 0x2b:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 0, 2, 0, 0, 0, 1, 0, 0};
                fill_control(pipe, signals);
                break;
            }

            // LW
            case 0x23:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 0, 2, 0, 0, 1, 0, 1, 1};
                fill_control(pipe, signals);
                break;
            }

            // BNE
            case 0x5:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 0, 1, 0, 6, 0, 1, 0, 0, 0, 0};
                fill_control(pipe, signals);
                break;
            }

            // BEQ
            case 0x4:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 0, 1, 0, 6, 1, 0, 0, 0, 0, 0};
                fill_control(pipe, signals);
                break;
            }

            // ANDI
            case 0xc:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1};
                fill_control(pipe, signals);
                break;
            }

            // LUI
            case 0xf:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 1, 9, 0, 0, 0, 0, 0, 1};
                fill_control(pipe, signals);
                break;
            }

            // ADDIU
            case 0x9:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 1, 2, 0, 0, 0, 0, 0, 1};
                fill_control(pipe, signals);
                break;
            }

            // SLTIU
            case 0xb:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 1, 7, 0, 0, 0, 0, 0, 1};
                fill_control(pipe, signals);
                break;
            }

            // ORI
            case 0xd:
            {
                uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1};
                fill_control(pipe, signals);
                break;
            }
        }
    }

    else if (type == J)
    {
        // J
        if(op_code == 0x2)
        {
            uint16_t signals[NUM_SIGNALS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            fill_control(pipe, signals);
            goto jump;
        }

        // JAL
        if (op_code == 0x3)
        {
            uint16_t signals[NUM_SIGNALS] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
            fill_control(pipe, signals);
            CURRENT_STATE.REGS[31] = CURRENT_STATE.PC - 4;
            goto jump;
        }

        jump:
        {
            CURRENT_STATE.JUMP_FLUSH = true;
            uint32_t target = TARGET(instr);
            CURRENT_STATE.PC = (target << 2);
        };
    }
}

bool detect_load_use_hazard()
{
    ID_EX_Pipe* id_ex = &CURRENT_STATE.ID_EX_PIPE;
    ID_EX_Pipe* past_id_ex = &CURRENT_STATE.PAST_ID_EX_PIPE;

    if (past_id_ex->mem_control.MemRead)
    {
        return ((id_ex->rs == past_id_ex->rt) || (id_ex->rt == past_id_ex->rt));
    }

    return false;
}

void set_forwarding(uint32_t* alu_1, uint32_t* alu_2, ID_EX_Pipe id_ex, EX_MEM_Pipe ex_mem, MEM_WB_Pipe mem_wb)
{
    uint32_t forwardA = 0;
    uint32_t forwardB = 0;

    if(ex_mem.wb_control.RegWrite && ex_mem.DEST)
    {
        if (ex_mem.DEST == id_ex.rs) forwardA = 2;

        if (ex_mem.DEST == id_ex.rt && !id_ex.ex_control.ALUSrc) forwardB = 2;
    }

    if (mem_wb.wb_control.RegWrite && mem_wb.DEST)
    {
        if (mem_wb.DEST == id_ex.rs) forwardA = 1;

        if (mem_wb.DEST == id_ex.rt && !id_ex.ex_control.ALUSrc) forwardB = 1;
    }

    *alu_1 = forwardA;
    *alu_2 = forwardB;
}

bool next_pipeline_empty()
{
    bool IF_empty = CURRENT_STATE.PIPE[IF_STAGE] == 0;
    bool ID_empty = CURRENT_STATE.PIPE[ID_STAGE] == 0;
    bool EX_empty = CURRENT_STATE.PIPE[EX_STAGE] == 0;
    bool MEM_empty = CURRENT_STATE.PIPE[MEM_STAGE] == 0;
    bool WB_empty = CURRENT_STATE.PIPE[WB_STAGE] == 0;

    return CYCLE_COUNT > 1 && IF_empty && ID_empty && EX_empty && MEM_empty && !WB_empty;
}

void switch_pipes()
{
    if (!CURRENT_STATE.EX_STALL)
        CURRENT_STATE.PAST_IF_ID_PIPE = CURRENT_STATE.IF_ID_PIPE;

    CURRENT_STATE.PAST_ID_EX_PIPE = CURRENT_STATE.ID_EX_PIPE;
    CURRENT_STATE.PAST_EX_MEM_PIPE = CURRENT_STATE.EX_MEM_PIPE;
    CURRENT_STATE.PAST_MEM_WB_PIPE = CURRENT_STATE.MEM_WB_PIPE;
}

void handle_flushes()
{
    if (CURRENT_STATE.JUMP_FLUSH)
    {
        flush_IF_ID(true);
        CURRENT_STATE.JUMP_FLUSH = false;
    }

    if (CURRENT_STATE.EX_STALL)
    {
        flush_ID_EX(true);
        CURRENT_STATE.EX_STALL = false;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/

instruction* get_inst_info(uint32_t pc) {
    if (CURRENT_STATE.PC >= (MEM_TEXT_START + NUM_INST * BYTES_PER_WORD))
        return NULL;

    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

void IF_Stage()
{
    instruction* instr = get_inst_info(CURRENT_STATE.PC);
    if (!instr) CURRENT_STATE.NEXT_PC = 0;
    else CURRENT_STATE.NEXT_PC = CURRENT_STATE.PC;

    CURRENT_STATE.PIPE[IF_STAGE] = CURRENT_STATE.NEXT_PC;
    CURRENT_STATE.IF_ID_PIPE.NPC = CURRENT_STATE.NEXT_PC;

    if (!CURRENT_STATE.PIPE[IF_STAGE]) return;

    CURRENT_STATE.IF_ID_PIPE.INST = instr;
    CURRENT_STATE.PC += BYTES_PER_WORD;
}

void ID_Stage()
{
    IF_ID_Pipe* past_if_id = &CURRENT_STATE.PAST_IF_ID_PIPE;
    ID_EX_Pipe* id_ex = &CURRENT_STATE.ID_EX_PIPE;
    instruction* instr = past_if_id->INST;

    CURRENT_STATE.PIPE[ID_STAGE] = past_if_id->NPC;
    id_ex->NPC = past_if_id->NPC;

    if (!CURRENT_STATE.PIPE[ID_STAGE]) return;

    encode_control_signals(instr);

    uint32_t reg1_val = CURRENT_STATE.REGS[RS(instr)];
    uint32_t reg2_val = CURRENT_STATE.REGS[RT(instr)];
    short imm = IMM(instr);
    short imm_sign_ext = SIGN_EX(imm);

    id_ex->REG1 = reg1_val;
    id_ex->REG2 = reg2_val;
    id_ex->IMM = imm_sign_ext;
    id_ex->rs = RS(instr);
    id_ex->rt = RT(instr);
    id_ex->rd = RD(instr);
    id_ex->shamt = SHAMT(instr);

    if(detect_load_use_hazard())
    {
        CURRENT_STATE.PC -= BYTES_PER_WORD;
        CURRENT_STATE.EX_STALL = true;
        id_ex->mem_control.MemWrite = 0;
        id_ex->wb_control.RegWrite = 0;
    }
}

void EX_Stage()
{
    EX_MEM_Pipe* ex_mem = &CURRENT_STATE.EX_MEM_PIPE;
    EX_MEM_Pipe* past_ex_mem = &CURRENT_STATE.PAST_EX_MEM_PIPE;
    MEM_WB_Pipe* past_mem_wb = &CURRENT_STATE.PAST_MEM_WB_PIPE;
    ID_EX_Pipe* past_id_ex = &CURRENT_STATE.PAST_ID_EX_PIPE;
    bool branchTaken;

    CURRENT_STATE.PIPE[EX_STAGE] = past_id_ex->NPC;
    ex_mem->NPC = past_id_ex->NPC;

    if (!CURRENT_STATE.PIPE[EX_STAGE]) return;

    uint32_t alu_in_1;
    uint32_t alu_in_2;
    uint32_t alu_out;
    uint32_t forwardA;
    uint32_t forwardB;

    unsigned char alu_op_0 = past_id_ex->ex_control.ALUop0;
    unsigned char alu_op_1 = past_id_ex->ex_control.ALUop1;
    unsigned char reg_dest = (past_id_ex->ex_control.RegDst) ? past_id_ex->rd : past_id_ex->rt;

    ex_mem->DEST = reg_dest;
    ex_mem->mem_control = past_id_ex->mem_control;
    ex_mem->wb_control = past_id_ex->wb_control;

    set_forwarding(&forwardA, &forwardB, *past_id_ex, *past_ex_mem, *past_mem_wb);

    if (forwardA == 0) alu_in_1 = past_id_ex->REG1;
    else if (forwardA == 1) alu_in_1 = (past_mem_wb->wb_control.MemtoReg) ? past_mem_wb->MEM_OUT : past_mem_wb->ALU_OUT;
    else if (forwardA == 2) alu_in_1 = past_ex_mem->ALU_OUT;

    if (forwardB == 0) alu_in_2 = (past_id_ex->ex_control.ALUSrc) ? past_id_ex->IMM : past_id_ex->REG2;
    else if (forwardB == 1) alu_in_2 = (past_mem_wb->wb_control.MemtoReg) ? past_mem_wb->MEM_OUT : past_mem_wb->ALU_OUT;
    else if (forwardB == 2) alu_in_2 = past_ex_mem->ALU_OUT;

    if (alu_op_1 == 0)
    {
        if (alu_op_0 == 0)
        {
            alu_out = alu_in_1 + alu_in_2;
        } else
        {
            alu_out = alu_in_1 - alu_in_2;
        }
    } else
    {
        switch(past_id_ex->ex_control.ALUCntrl)
        {
            case 0:
            {
                alu_out = alu_in_1 & alu_in_2;
                break;
            }
            case 1:
            {
                alu_out = alu_in_1 | alu_in_2;
                break;
            }
            case 2:
            {
                alu_out = alu_in_1 + alu_in_2;
                break;
            }
            case 3:
            {
                uint32_t shamt = past_id_ex->shamt;
                alu_out = alu_in_2 << shamt;
                break;
            }
            case 4:
            {
                uint32_t shamt = past_id_ex->shamt;
                alu_out = alu_in_2 >> shamt;
                break;
            }
            case 6:
            {
                alu_out = alu_in_1 - alu_in_2;
                break;
            }
            case 7:
            {
                alu_out = alu_in_1 < alu_in_2;
                break;
            }
            case 9:
            {
                alu_out = alu_in_2 << 16;
                break;
            }

            case 12:
            {
                alu_out = ~(alu_in_1 | alu_in_2);
                break;
            }
        }
    }

    ex_mem->ALU_OUT = alu_out;
    branchTaken = ((!alu_out && past_id_ex->mem_control.Branch) || (alu_out && past_id_ex->mem_control.BranchNE));

    if (branchTaken)
    {
        uint32_t branch_target = past_id_ex->NPC + (past_id_ex->IMM * BYTES_PER_WORD) + BYTES_PER_WORD;
        ex_mem->BR_TAKEN = 1;
        ex_mem->BR_TARGET = branch_target;
        CURRENT_STATE.PC = ex_mem->BR_TARGET;
    }
}

void MEM_Stage()
{
    EX_MEM_Pipe* past_ex_mem = &CURRENT_STATE.PAST_EX_MEM_PIPE;
    MEM_WB_Pipe* mem_wb = &CURRENT_STATE.MEM_WB_PIPE;

    CURRENT_STATE.PIPE[MEM_STAGE] = past_ex_mem->NPC;
    mem_wb->NPC = past_ex_mem->NPC;

    if (past_ex_mem->BR_TAKEN)
    {
        flush_IF_ID(false);
        flush_ID_EX(false);
        flush_EX_MEM();
        CURRENT_STATE.PC -= BYTES_PER_WORD;
    }

    if (!CURRENT_STATE.PIPE[MEM_STAGE]) return;

    mem_wb->DEST = past_ex_mem->DEST;
    mem_wb->wb_control = past_ex_mem->wb_control;

    if (past_ex_mem->mem_control.MemRead)
    {
        mem_wb->MEM_OUT = mem_read_32(past_ex_mem->ALU_OUT);
    } else
    {
        mem_wb->ALU_OUT = past_ex_mem->ALU_OUT;
    }

    if (past_ex_mem->mem_control.MemWrite)
    {
        mem_write_32(past_ex_mem->ALU_OUT, past_ex_mem->DEST);
    }
}

void WB_Stage()
{
    MEM_WB_Pipe* past_mem_wb = &CURRENT_STATE.PAST_MEM_WB_PIPE;
    CURRENT_STATE.PIPE[WB_STAGE] = past_mem_wb->NPC;

    if (!CURRENT_STATE.PIPE[WB_STAGE]) return;

    past_mem_wb->WRITE_VALUE = (past_mem_wb->wb_control.MemtoReg) ? past_mem_wb->MEM_OUT : past_mem_wb->ALU_OUT;

    if (past_mem_wb->wb_control.RegWrite)
    {
        CURRENT_STATE.REGS[past_mem_wb->DEST] = past_mem_wb->WRITE_VALUE;
    }

    if (CURRENT_STATE.PIPE[WB_STAGE])
    {
        INSTRUCTION_COUNT++;
    }

    CURRENT_STATE.REGS[0] = 0;
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/

void process_instruction() {
    IF_Stage();
    WB_Stage();
    ID_Stage();
    EX_Stage();
    MEM_Stage();

    switch_pipes();
    handle_flushes();

    RUN_BIT = !next_pipeline_empty();
}