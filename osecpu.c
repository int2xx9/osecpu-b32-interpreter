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

int do_instruction(struct Osecpu* osecpu, const int icode)
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
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] | osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_XOR:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] ^ osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_AND:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] & osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_ADD:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] + osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_SUB:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] - osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_MUL:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] * osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_SHL:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] << osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_SAR:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] >> osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_DIV:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				if (osecpu->registers[r2] == 0) goto division_by_zero_error;
				osecpu->registers[r0] = osecpu->registers[r1] / osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_MOD:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				if (osecpu->registers[r2] == 0) goto division_by_zero_error;
				osecpu->registers[r0] = osecpu->registers[r1] % osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_CMPE:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int bit1 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit0 = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit1 != 0x20) goto invalid_argument_error;
				if (bit0 != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1]==osecpu->registers[r2] ? -1 : 0;
			}
			break;
		case INSTRUCTION_CMPNE:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int bit1 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit0 = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit1 != 0x20) goto invalid_argument_error;
				if (bit0 != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1]!=osecpu->registers[r2] ? -1 : 0;
			}
			break;
		case INSTRUCTION_CMPL:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int bit1 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit0 = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit1 != 0x20) goto invalid_argument_error;
				if (bit0 != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1]<osecpu->registers[r2] ? -1 : 0;
			}
			break;
		case INSTRUCTION_CMPGE:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int bit1 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit0 = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit1 != 0x20) goto invalid_argument_error;
				if (bit0 != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1]>=osecpu->registers[r2] ? -1 : 0;
			}
			break;
		case INSTRUCTION_CMPLE:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int bit1 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit0 = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit1 != 0x20) goto invalid_argument_error;
				if (bit0 != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1]<=osecpu->registers[r2] ? -1 : 0;
			}
			break;
		case INSTRUCTION_CMPG:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int bit1 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit0 = fetch_code(osecpu)-0x76000000;
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit1 != 0x20) goto invalid_argument_error;
				if (bit0 != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1]>osecpu->registers[r2] ? -1 : 0;
			}
			break;
		default:
			osecpu->invalid_instruction_error = icode;
			return -1;
	}
	return 0;

invalid_argument_error:
	osecpu->invalid_argument_error = 1;
	return -1;

division_by_zero_error:
	osecpu->division_by_zero_error = 1;
	return -1;
}

int run_b32(struct Osecpu* osecpu)
{
	int icode;
	while (1) {
		icode = fetch_code(osecpu);
		if (osecpu->codelen <= osecpu->pregisters[0x3f]) break;
		if (do_instruction(osecpu, icode) == -1) return -1;
	}
	return 0;
}

