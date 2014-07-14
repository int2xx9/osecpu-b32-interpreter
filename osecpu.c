#include "osecpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define INSTRUCTION_LIMM	0x76000002
#define INSTRUCTION_OR		0x76000010
#define INSTRUCTION_XOR		0x76000011
#define INSTRUCTION_AND		0x76000012
#define INSTRUCTION_ADD		0x76000014
#define INSTRUCTION_SUB		0x76000015
#define INSTRUCTION_MUL		0x76000016
#define INSTRUCTION_SHL		0x76000018
#define INSTRUCTION_SAR		0x76000019
#define INSTRUCTION_DIV		0x7600001a
#define INSTRUCTION_MOD		0x7600001b
#define INSTRUCTION_CMPE	0x76000020
#define INSTRUCTION_CMPNE	0x76000021
#define INSTRUCTION_CMPL	0x76000022
#define INSTRUCTION_CMPGE	0x76000023
#define INSTRUCTION_CMPLE	0x76000024
#define INSTRUCTION_CMPG	0x76000025
#define INSTRUCTION_LIDR	0x760000fd

struct Osecpu* init_osecpu()
{
	struct Osecpu* osecpu;
	osecpu = (struct Osecpu*)malloc(sizeof(struct Osecpu));
	if (!osecpu) return NULL;
	memset(osecpu, 0, sizeof(struct Osecpu));
	return osecpu;
}

void free_osecpu(struct Osecpu* osecpu)
{
	if (osecpu->code) free(osecpu->code);
	free(osecpu);
}

int fetch_b32code(const uint8_t* code, const int base, const int len, int* ret_value)
{
	int i;
	int offset;
	int fetch_bytes;

	if (strncmp(code+base, "\x76", 1) == 0) {
		offset = 1;
		fetch_bytes = 3;
	} else if (strncmp(code+base, "\xff\xff\xf7\x88", 4) == 0) {
		offset = 4;
		fetch_bytes = 4;
	} else {
		return 0;
	}

	// Validate buffer overflow
	if (base+offset+fetch_bytes > len) {
		return 0;
	}

	*ret_value = 0;
	for (i = 0; i < fetch_bytes; i++) {
		*ret_value = (*ret_value << 8) | code[base+offset+i];
	}

	return offset+fetch_bytes;
}

int fetch_b32instruction(const uint8_t* code, const int base, const int len, struct Instruction* inst, int* error)
{
	int inc;
	int ret;

	*error = 0;
	inc = ret = fetch_b32code(code, base, len, (int*)&inst->id);
	if (ret == 0) {
		if (base+inc >= len) {
			*error = 0;
		} else {
			*error = ERROR_INVALID_B32_CODE;
		}
		return 0;
	}
	switch (inst->id)
	{
		case LIMM:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.limm.imm);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.limm.r);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.limm.bit);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_REGISTER_ID(inst->arg.limm.r)) goto invalid_argument_error;
			if (inst->arg.limm.bit != 0x20) goto invalid_argument_error;
			break;
		case LIDR:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lidr.imm);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lidr.dr);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_DREGISTER_ID(inst->arg.lidr.dr)) goto invalid_argument_error;
			break;
		case OR:
		case XOR:
		case AND:
		case ADD:
		case SUB:
		case MUL:
		case SHL:
		case SAR:
		case DIV:
		case MOD:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.operate.r1);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.operate.r2);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.operate.r0);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.operate.bit);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_REGISTER_ID(inst->arg.operate.r1)) goto invalid_argument_error;
			if (!IS_VALID_REGISTER_ID(inst->arg.operate.r2)) goto invalid_argument_error;
			if (!IS_VALID_REGISTER_ID(inst->arg.operate.r0)) goto invalid_argument_error;
			if (inst->arg.operate.bit != 0x20) goto invalid_argument_error;
			break;
		case CMPE:
		case CMPNE:
		case CMPL:
		case CMPGE:
		case CMPLE:
		case CMPG:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.compare.r1);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.compare.r2);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.compare.bit1);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.compare.r0);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.compare.bit0);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_REGISTER_ID(inst->arg.compare.r1)) goto invalid_argument_error;
			if (!IS_VALID_REGISTER_ID(inst->arg.compare.r2)) goto invalid_argument_error;
			if (inst->arg.compare.bit1 != 0x20) goto invalid_argument_error;
			if (!IS_VALID_REGISTER_ID(inst->arg.compare.r0)) goto invalid_argument_error;
			if (inst->arg.compare.bit0 != 0x20) goto invalid_argument_error;
			break;
		default:
			*error = ERROR_INVALID_INSTRUCTION;
			return 0;
	}
	return inc;

fetch_b32code_error:
	if (inc >= len) {
		*error = ERROR_UNEXPECTED_EOC;
	} else {
		*error = ERROR_INVALID_B32_CODE;
	}
	return 0;

invalid_argument_error:
	*error = ERROR_INVALID_ARGUMENT;
	return 0;
}

int count_instructions(const uint8_t* code, const int len, int* error)
{
	int codepos = 0;
	struct Instruction inst;
	int ret;
	int instcnt = 0;
	*error = 0;

	while (1)
	{
		codepos += ret = fetch_b32instruction(code, codepos, len, &inst, error);
		if (ret == 0) break;
		instcnt++;
	}

	return instcnt;
}

int load_b32_from_file(struct Osecpu* osecpu, const char* filename)
{
	FILE* fp;
	long len;
	uint8_t* code;

	if (osecpu->code) return -1;

	fp = fopen(filename, "rb");
	if (!fp) return -1;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// ファイルの長さが8バイト以下(シグネチャ長以下)はエラー
	if (len <= 8) {
		fclose(fp);
		return -1;
	}

	code = (uint8_t*)malloc(len);
	if (!code) {
		fclose(fp);
		return -1;
	}

	if (fread(code, len, 1, fp) < 1) {
		free(code);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	// シグネチャチェック
	if (strcmp(code, "\x05\xe2\x00\xcf\xee\x7f\xf1\x88") != 0) {
		free(code);
		return -1;
	}

	if (load_b32_from_memory(osecpu, code+8, len-8) != 0) return -1;

	free(code);
	return 0;
}

int load_b32_from_memory(struct Osecpu* osecpu, const uint8_t* code, long len)
{
	if (osecpu->code) return -1;

	osecpu->code = (uint8_t*)malloc(len);
	if (!osecpu->code) return -1;
	osecpu->codelen = len;
	memcpy(osecpu->code, code, len);

	return 0;
}

void coredump(struct Osecpu* osecpu)
{
	int i, j;

	printf("Registers:\n");
	i = 0;
	while (i < 0x40/4) {
		printf("R%02X: %08X\tR%02X: %08X\tR%02X: %08X\tR%02X: %08X\n"
				, i+16*0, osecpu->registers[i+16*0]
				, i+16*1, osecpu->registers[i+16*1]
				, i+16*2, osecpu->registers[i+16*2]
				, i+16*3, osecpu->registers[i+16*3]
			  );
		i++;
	}
	printf("\n");

	printf("Registers(pointer):\n");
	i = 0;
	while (i < 0x40/4) {
		printf("P%02X: %08X\tP%02X: %08X\tP%02X: %08X\tP%02X: %08X\n"
				, i+16*0, osecpu->pregisters[i+16*0]
				, i+16*1, osecpu->pregisters[i+16*1]
				, i+16*2, osecpu->pregisters[i+16*2]
				, i+16*3, osecpu->pregisters[i+16*3]
			  );
		i++;
	}
	printf("\n");

	printf("Registers(debug):\n");
	for (i = 0; i < 4; i++) {
		printf("DR%X: %08X\t", i, osecpu->dregisters[i]);
	}
	printf("\n\n");

	printf("osecpu->code:\n");
	i = 0;
	while (i < osecpu->codelen) {
		printf("%08X: ", i);
		for (j = 0; j < 16; j++) {
			if (i+j > osecpu->codelen-1) break;
			printf("%02X ", osecpu->code[i+j]);
		}
		printf("\n");
		i+=j;
	}
}

int fetch_code(struct Osecpu* osecpu)
{
	const int pc = osecpu->pregisters[0x3f];
	if (pc+4 > osecpu->codelen) return 0;
	osecpu->pregisters[0x3f] += 4;
	return
		(osecpu->code[pc+0] << 24) |
		(osecpu->code[pc+1] << 16) |
		(osecpu->code[pc+2] <<  8) |
		(osecpu->code[pc+3] <<  0);
}

void do_operate_instruction(struct Osecpu* osecpu, const int icode)
{
	const int r1 = fetch_code(osecpu)-0x76000000;
	const int r2 = fetch_code(osecpu)-0x76000000;
	const int r0 = fetch_code(osecpu)-0x76000000;
	const int bit = fetch_code(osecpu)-0x76000000;

	if (r1 < 0 || r1 > 0x3f) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (r2 < 0 || r2 > 0x3f) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (r0 < 0 || r0 > 0x3f) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (bit != 0x20) osecpu->error = ERROR_INVALID_ARGUMENT;

	if (osecpu->error != 0) return;

	switch (icode) {
		case INSTRUCTION_OR:
			osecpu->registers[r0] = osecpu->registers[r1] | osecpu->registers[r2];
			break;
		case INSTRUCTION_XOR:
			osecpu->registers[r0] = osecpu->registers[r1] ^ osecpu->registers[r2];
			break;
		case INSTRUCTION_AND:
			osecpu->registers[r0] = osecpu->registers[r1] & osecpu->registers[r2];
			break;
		case INSTRUCTION_ADD:
			osecpu->registers[r0] = osecpu->registers[r1] + osecpu->registers[r2];
			break;
		case INSTRUCTION_SUB:
			osecpu->registers[r0] = osecpu->registers[r1] - osecpu->registers[r2];
			break;
		case INSTRUCTION_MUL:
			osecpu->registers[r0] = osecpu->registers[r1] * osecpu->registers[r2];
			break;
		case INSTRUCTION_SHL:
			osecpu->registers[r0] = osecpu->registers[r1] << osecpu->registers[r2];
			break;
		case INSTRUCTION_SAR:
			osecpu->registers[r0] = osecpu->registers[r1] >> osecpu->registers[r2];
			break;
		case INSTRUCTION_DIV:
		case INSTRUCTION_MOD:
			if (osecpu->registers[r2] == 0) {
				osecpu->error = ERROR_DIVISION_BY_ZERO;
			} else if (icode == INSTRUCTION_DIV) {
				osecpu->registers[r0] = osecpu->registers[r1] / osecpu->registers[r2];
			} else if (icode == INSTRUCTION_MOD) {
				osecpu->registers[r0] = osecpu->registers[r1] % osecpu->registers[r2];
			}
			break;
	}
}

void do_compare_instruction(struct Osecpu* osecpu, const int icode)
{
	const int r1 = fetch_code(osecpu)-0x76000000;
	const int r2 = fetch_code(osecpu)-0x76000000;
	const int bit1 = fetch_code(osecpu)-0x76000000;
	const int r0 = fetch_code(osecpu)-0x76000000;
	const int bit0 = fetch_code(osecpu)-0x76000000;

	if (r1 < 0 || r1 > 0x3f) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (r2 < 0 || r2 > 0x3f) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (r0 < 0 || r0 > 0x3f) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (bit1 != 0x20) osecpu->error = ERROR_INVALID_ARGUMENT;
	if (bit0 != 0x20) osecpu->error = ERROR_INVALID_ARGUMENT;

	if (osecpu->error != 0) return;

	switch(icode) {
		case INSTRUCTION_CMPE:
			osecpu->registers[r0] = osecpu->registers[r1]==osecpu->registers[r2] ? -1 : 0;
			break;
		case INSTRUCTION_CMPNE:
			osecpu->registers[r0] = osecpu->registers[r1]!=osecpu->registers[r2] ? -1 : 0;
			break;
		case INSTRUCTION_CMPL:
			osecpu->registers[r0] = osecpu->registers[r1]<osecpu->registers[r2] ? -1 : 0;
			break;
		case INSTRUCTION_CMPGE:
			osecpu->registers[r0] = osecpu->registers[r1]>=osecpu->registers[r2] ? -1 : 0;
			break;
		case INSTRUCTION_CMPLE:
			osecpu->registers[r0] = osecpu->registers[r1]<=osecpu->registers[r2] ? -1 : 0;
			break;
		case INSTRUCTION_CMPG:
			osecpu->registers[r0] = osecpu->registers[r1]>osecpu->registers[r2] ? -1 : 0;
			break;
	}
}

void do_instruction(struct Osecpu* osecpu, const int icode)
{
	switch (icode) {
		case INSTRUCTION_LIMM:
			{
				const int resv = fetch_code(osecpu);
				const int imm = fetch_code(osecpu);
				const int r = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (resv != 0xfffff788) goto invalid_argument_error;
				if (r < 0 || r > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r] = imm;
			}
			break;
		case INSTRUCTION_LIDR:
			{
				const int resv = fetch_code(osecpu);
				const int imm = fetch_code(osecpu);
				const int dr = fetch_code(osecpu)-0x76000000;
				if (resv != 0xfffff788) goto invalid_argument_error;
				if (dr < 0 || dr > 4) goto invalid_argument_error;
				osecpu->dregisters[dr] = imm;
			}
			break;
		case INSTRUCTION_OR:
		case INSTRUCTION_XOR:
		case INSTRUCTION_AND:
		case INSTRUCTION_ADD:
		case INSTRUCTION_SUB:
		case INSTRUCTION_MUL:
		case INSTRUCTION_SHL:
		case INSTRUCTION_SAR:
		case INSTRUCTION_DIV:
		case INSTRUCTION_MOD:
			do_operate_instruction(osecpu, icode);
			break;
		case INSTRUCTION_CMPE:
		case INSTRUCTION_CMPNE:
		case INSTRUCTION_CMPL:
		case INSTRUCTION_CMPGE:
		case INSTRUCTION_CMPLE:
		case INSTRUCTION_CMPG:
			do_compare_instruction(osecpu, icode);
			break;
		default:
			osecpu->error = ERROR_INVALID_INSTRUCTION;
			break;
	}
	return;

invalid_argument_error:
	osecpu->error = ERROR_INVALID_ARGUMENT;
	return;
}

int run_b32(struct Osecpu* osecpu)
{
	int icode;
	while (1) {
		icode = fetch_code(osecpu);
		if (osecpu->codelen <= osecpu->pregisters[0x3f]) break;
		do_instruction(osecpu, icode);
		if (osecpu->error != 0) return -1;
	}
	return 0;
}

