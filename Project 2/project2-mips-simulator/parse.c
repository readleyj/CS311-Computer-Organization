/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "parse.h"

int text_size;
int data_size;

instruction parsing_instr(const char *buffer, const int index)
{
    instruction instr;
    uint32_t instruction_val = fromBinary(buffer);
    mem_write_32(MEM_TEXT_START + index, instruction_val);

    char op_code_char[6];
    memcpy(op_code_char, buffer, 6);
    short op_code_int = fromBinary(op_code_char);
    instr.opcode = op_code_int;

    switch(op_code_int)
    {
        case 0x0:
            {
                char rs_char[6], rt_char[6], rd_char[6], shamt_char[6], funct_char[7];

                memcpy(rs_char, buffer + 6, 5);
                memcpy(rt_char, buffer + 11, 5);
                memcpy(rd_char, buffer + 16, 5);
                memcpy(shamt_char, buffer + 21, 5);
                memcpy(funct_char, buffer + 26, 6);

                rs_char[5] = '\0';
                rt_char[5] = '\0';
                rd_char[5] = '\0';
                shamt_char[5] = '\0';
                funct_char[6] = '\0';

                short funct_code = fromBinary(funct_char);
                unsigned char rs_code = fromBinary(rs_char);
                unsigned char rt_code = fromBinary(rt_char);
                unsigned char rd_code = fromBinary(rd_char);
                unsigned char shamt_code = fromBinary(shamt_char);

                instr.func_code = funct_code;
                instr.r_t.r_i.rs = rs_code;
                instr.r_t.r_i.rt = rt_code;
                instr.r_t.r_i.r_i.r.rd = rd_code;
                instr.r_t.r_i.r_i.r.shamt = shamt_code;
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
            char rs_char[6], rt_char[6], imm_char[17];
            memcpy(rs_char, buffer + 6, 5);
            memcpy(rt_char, buffer + 11, 5);
            memcpy(imm_char, buffer + 16, 16);

            rs_char[5] = '\0';
            rt_char[5] = '\0';
            imm_char[16] = '\0';

            unsigned char rs_code = fromBinary(rs_char);
            unsigned char rt_code = fromBinary(rt_char);
            short imm = fromBinary(imm_char);

            instr.r_t.r_i.rs = rs_code;
            instr.r_t.r_i.rt = rt_code;
            instr.r_t.r_i.r_i.imm = imm;
            break;
        }

        case 0x2:
        case 0x3:
        {
            char target_char[27];
            memcpy(target_char, buffer + 6, 26);
            target_char[26] = '\0';

            uint32_t target = fromBinary(target_char);
            instr.r_t.target = target;
            break;
        }
    }

    return instr;
}

void parsing_data(const char *buffer, const int index)
{
	uint32_t data_val = fromBinary(buffer);
	mem_write_32(MEM_DATA_START + index, data_val);
}

void print_parse_result()
{
    int i;
    printf("Instruction Information\n");

    for(i = 0; i < text_size/4; i++)
    {
	printf("INST_INFO[%d].value : %x\n",i, INST_INFO[i].value);
	printf("INST_INFO[%d].opcode : %d\n",i, INST_INFO[i].opcode);

	switch(INST_INFO[i].opcode)
	{
	    //Type I
	    case 0x9:		//(0x001001)ADDIU
	    case 0xc:		//(0x001100)ANDI
	    case 0xf:		//(0x001111)LUI	
	    case 0xd:		//(0x001101)ORI
	    case 0xb:		//(0x001011)SLTIU
	    case 0x23:		//(0x100011)LW	
	    case 0x2b:		//(0x101011)SW
	    case 0x4:		//(0x000100)BEQ
	    case 0x5:		//(0x000101)BNE
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].imm : %d\n",i, INST_INFO[i].r_t.r_i.r_i.imm);
		break;

    	    //TYPE R
	    case 0x0:		//(0x000000)ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU  if JR
		printf("INST_INFO[%d].func_code : %d\n",i, INST_INFO[i].func_code);
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].rd : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.rd);
		printf("INST_INFO[%d].shamt : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.shamt);
		break;

    	    //TYPE J
	    case 0x2:		//(0x000010)J
	    case 0x3:		//(0x000011)JAL
		printf("INST_INFO[%d].target : %d\n",i, INST_INFO[i].r_t.target);
		break;

	    default:
		printf("Not available instruction\n");
		assert(0);
	}
    }

    printf("Memory Dump - Text Segment\n");
    for(i = 0; i < text_size; i+=4)
	printf("text_seg[%d] : %x\n", i, mem_read_32(MEM_TEXT_START + i));
    for(i = 0; i < data_size; i+=4)
	printf("data_seg[%d] : %x\n", i, mem_read_32(MEM_DATA_START + i));
    printf("Current PC: %x\n", CURRENT_STATE.PC);
}
