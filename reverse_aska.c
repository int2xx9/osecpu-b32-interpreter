#include "reverse_aska.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B32_SIGNATURE "\x05\xe2\x00\xcf\xee\x7f\xf1\x88"

int fetch_b32value(const unsigned char* code, int* pos, int code_bytes)
{
	int i;
	int offset;
	int fetch_bytes;
	int ret_value;

	if (*pos < 0) return 0;

	if (code[*pos] == 0x76) {
		offset = 1;
		fetch_bytes = 3;
	} else if (strncmp(code+*pos, "\xff\xff\xf7\x88", 4) == 0) {
		offset = 4;
		fetch_bytes = 4;
	} else {
		*pos = -1;
		return 0;
	}

	// TODO: オーバーフローチェックがない

	ret_value = 0;
	for (i = 0; i < fetch_bytes; i++) {
		ret_value = (ret_value << 8) | code[*pos+offset+i];
	}

	*pos += offset+fetch_bytes;
	return ret_value;
}

int reverse_aska_prepare_code(ReverseAska* raska)
{
	int* instruction_index;
	int instruction_index_cnt;
	int idxcnt;
	int pos;
	enum InstructionId instid;
	int skipcnt;

	instruction_index_cnt = 64;
	instruction_index = (int*)malloc(sizeof(int) * instruction_index_cnt);
	if (!instruction_index) return 0;

	idxcnt = 0;
	pos = 8;
	do {
		if (instruction_index_cnt >= idxcnt) {
			instruction_index_cnt += 64;
			instruction_index = (int*)realloc(instruction_index, sizeof(int) * instruction_index_cnt);
			if (!instruction_index) return 0;
		}

		instruction_index[idxcnt++] = pos;

		instid = fetch_b32value(raska->code, &pos, raska->codelen);
		if (pos == -1) goto error;

		switch (instid) {
			case CND:
				skipcnt = 1;
				break;
			case LB:
			case PLIMM:
			case PCP:
			case LIDR:
				skipcnt = 2;
				break;
			case LIMM:
				skipcnt = 3;
				break;
			case OR:
			case XOR:
			case AND:
			case SBX:
			case ADD:
			case SUB:
			case MUL:
			case SHL:
			case SAR:
			case DIV:
			case MOD:
				skipcnt = 4;
				break;
			case LMEM:
			case SMEM:
			case PADD:
			case CMPE:
			case CMPNE:
			case CMPL:
			case CMPGE:
			case CMPLE:
			case CMPG:
			case TSTZ:
			case TSTNZ:
				skipcnt = 5;
				break;
			case DATA:
				{
					int typ = fetch_b32value(raska->code, &pos, raska->codelen);
					int len = fetch_b32value(raska->code, &pos, raska->codelen);
					skipcnt = 0;
					pos += len*4;
				}
				break;
			case REM:
				{
					int uimm = fetch_b32value(raska->code, &pos, raska->codelen);
					switch (uimm)
					{
						case 0x00: skipcnt = 1; break;
						case 0x01: skipcnt = 1; break;
						case 0x02: skipcnt = 2; break;
						case 0x03: skipcnt = 1; break;
						case 0x34: skipcnt = 1; break;
						default: goto error; break;
					}
				}
				break;
			default:
				goto error;
		}
		while (skipcnt-- > 0) {
			fetch_b32value(raska->code, &pos, raska->codelen);
		}
	} while (pos < raska->codelen);

	raska->instruction_index = instruction_index;
	raska->idxcnt = idxcnt;
	return 1;

error:
	free(instruction_index);
	return 0;
}

ReverseAska* reverse_aska_init(const uint8_t* code, int codelen)
{
	ReverseAska* raska = NULL;

	if (strncmp(code, B32_SIGNATURE, 8) != 0) return NULL;

	raska = (ReverseAska*)malloc(sizeof(ReverseAska));
	if (!raska) goto error;
	memset(raska, 0, sizeof(ReverseAska));

	raska->code = (uint8_t*)malloc(codelen);
	if (!raska->code) goto error;
	memcpy(raska->code, code, codelen);
	raska->codelen = codelen;

	if (!reverse_aska_prepare_code(raska)) goto error;

	return raska;

error:
	free(raska->code);
	free(raska->instruction_index);
	free(raska);
	return NULL;
}

void reverse_aska_free(ReverseAska* raska)
{
	free(raska->code);
	free(raska->instruction_index);
	free(raska);
}

struct ReverseAskaInstruction* get_instruction_string(ReverseAska* raska, int num)
{
	struct ReverseAskaInstruction* ret_inst = NULL;
	int pos;
	int instpos = 0, instnum = 0;
	enum InstructionId instid;
	static const char* operate_inst_name[] = {
		"OR", "XOR", "AND", "SBX",
		"ADD", "SUB", "MUL",
		"SHL", "SAR", "DIV", "MOD",
	};
	static const char* compare_inst_name[] = {
		"CMPE", "CMPNE", "CMPL", "CMPGE",
		"CMPLE", "CMPG", "TSTZ", "TSTNZ",
	};

	if (num < 0 || num >= raska->idxcnt) return NULL;

	ret_inst = (struct ReverseAskaInstruction*)malloc(sizeof(struct ReverseAskaInstruction));
	if (!ret_inst) return NULL;
	// FIXME: めんどくさいのでとりあえずどれも100バイト確保してある
	ret_inst->inst_str = (char*)malloc(sizeof(char) * 100);
	if (!ret_inst->inst_str) {
		free(ret_inst);
		return NULL;
	}
	ret_inst->number = num;

	pos = raska->instruction_index[num];
	instid = fetch_b32value(raska->code, &pos, raska->codelen);

	switch (instid) {
		case LB:
			{
				int uimm = fetch_b32value(raska->code, &pos, raska->codelen);
				int opt = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "LB(opt:%d, uimm:%d);", opt, uimm);
			}
			break;
		case LIMM:
			{
				int imm = fetch_b32value(raska->code, &pos, raska->codelen);
				int r = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "LIMM(bit:%d, r:R%02X, imm:0x%08x);", bit, r, imm);
			}
			break;
		case PLIMM:
			{
				int uimm = fetch_b32value(raska->code, &pos, raska->codelen);
				int p = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "PLIMM(p:P%02X, uimm:%d);", p, uimm);
			}
			break;
		case CND:
			{
				int r = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "CND(r:R%02X);", r);
			}
			break;
		case LMEM:
			{
				int p = fetch_b32value(raska->code, &pos, raska->codelen);
				int typ = fetch_b32value(raska->code, &pos, raska->codelen);
				int zero = fetch_b32value(raska->code, &pos, raska->codelen);
				int r = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "LMEM(bit:%d, r:R%02X, typ:%d, p:P%02X, %d);", bit, r, typ, p, zero);
			}
			break;
		case SMEM:
			{
				int r = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit = fetch_b32value(raska->code, &pos, raska->codelen);
				int p = fetch_b32value(raska->code, &pos, raska->codelen);
				int typ = fetch_b32value(raska->code, &pos, raska->codelen);
				int zero = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "SMEM(bit:%d, r:R%02X, typ:%d, p:P%02X, %d);", bit, r, typ, p, zero);
			}
			break;
		case PADD:
			{
				int p1 = fetch_b32value(raska->code, &pos, raska->codelen);
				int typ = fetch_b32value(raska->code, &pos, raska->codelen);
				int r = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit = fetch_b32value(raska->code, &pos, raska->codelen);
				int p0 = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "PADD(bit:%d, p0]P%02X, typ:%d, p1:P%02X, r:R%02X);", bit, p0, typ, p1, r);
			}
			break;
		case OR:
		case XOR:
		case AND:
		case SBX:
		case ADD:
		case SUB:
		case MUL:
		case SHL:
		case SAR:
		case DIV:
		case MOD:
			{
				int r1 = fetch_b32value(raska->code, &pos, raska->codelen);
				int r2 = fetch_b32value(raska->code, &pos, raska->codelen);
				int r0 = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit = fetch_b32value(raska->code, &pos, raska->codelen);
				if (instid == OR && r1 == r2) {
					snprintf(ret_inst->inst_str, 100, "CP(r0:R%02X, r1:R%02X);", r0, r1);
				} else {
					snprintf(ret_inst->inst_str, 100, "%s(bit:%d, r0:R%02X, r1:R%02X, r2:R%02X);", operate_inst_name[instid-OR], bit, r0, r1, r2);
				}
			}
			break;
		case CMPE:
		case CMPNE:
		case CMPL:
		case CMPGE:
		case CMPLE:
		case CMPG:
		case TSTZ:
		case TSTNZ:
			{
				int r1 = fetch_b32value(raska->code, &pos, raska->codelen);
				int r2 = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit1 = fetch_b32value(raska->code, &pos, raska->codelen);
				int r0 = fetch_b32value(raska->code, &pos, raska->codelen);
				int bit0 = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "%s(bit0:%d, bit1:%d, r0:R%02X, r1:R%02X, r2:R%02X);", compare_inst_name[instid-CMPE], bit0, bit1, r0, r1, r2);
			}
			break;
		case PCP:
			{
				int p1 = fetch_b32value(raska->code, &pos, raska->codelen);
				int p0 = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "PCP(p0:P%02X, p1:P%02X);", p0, p1);
			}
			break;
		case DATA:
			{
				int typ = fetch_b32value(raska->code, &pos, raska->codelen);
				int len = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "data(typ:%d, len:%d);", typ, len);
			}
			break;
		case LIDR:
			{
				int imm = fetch_b32value(raska->code, &pos, raska->codelen);
				int dr = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "LIDR(dr:D%02X, imm:%d);", dr, imm);
			}
			break;
		case REM:
			{
				int uimm = fetch_b32value(raska->code, &pos, raska->codelen);
				snprintf(ret_inst->inst_str, 100, "REM%02X(...);", uimm);
			}
			break;
		default:
			snprintf(ret_inst->inst_str, 100, "(unknown:%x)", instid);
			free(ret_inst->inst_str);
			free(ret_inst);
			return NULL;
	}

	return ret_inst;
}

