#include "osecpu.h"
#include "api.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define BIG_TO_LITTLE(val) \
	( \
		(unsigned)((val)&0xff000000) >> 24 | \
		(unsigned)((val)&0x00ff0000) >>  8 | \
		(unsigned)((val)&0x0000ff00) <<  8 | \
		(unsigned)((val)&0x000000ff) << 24 \
	)

#define LABEL_API 0xffffffff
#define B32_SIGNATURE "\x05\xe2\x00\xcf\xee\x7f\xf1\x88"

const char* get_error_text(int id)
{
	if (id < 1 || id > sizeof(ErrorMessages)/sizeof(ErrorMessages[0])) return 0;
	return ErrorMessages[id];
}

struct Osecpu* init_osecpu()
{
	struct Osecpu* osecpu;
	osecpu = (struct Osecpu*)malloc(sizeof(struct Osecpu));
	if (!osecpu) return NULL;
	memset(osecpu, 0, sizeof(struct Osecpu));
	// instruction pointer
	osecpu->pregisters[0x3f].type = CODE;
	// a pointer to call APIs
	osecpu->pregisters[0x2f].type = CODE;
	osecpu->pregisters[0x2f].p.code = LABEL_API;
	return osecpu;
}

void free_osecpu(struct Osecpu* osecpu)
{
	if (osecpu->code) free(osecpu->code);
	if (osecpu->labels) free(osecpu->labels);
	if (osecpu->window) window_free(osecpu->window);
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
		case NOP:
			// No arguments
			break;
		case LB:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lb.uimm);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lb.opt);
			if (ret == 0) goto fetch_b32code_error;

			if (inst->arg.lb.opt < 0 || inst->arg.lb.opt > 3) goto invalid_argument_error;
			break;
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
		case PLIMM:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.plimm.uimm);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.plimm.p);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_PREGISTER_ID(inst->arg.plimm.p)) goto invalid_argument_error;
			break;
		case CND:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.cnd.r);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_REGISTER_ID(inst->arg.cnd.r)) goto invalid_argument_error;
			break;
		case LMEM:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lmem.p);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lmem.typ);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lmem.zero);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lmem.r);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.lmem.bit);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_PREGISTER_ID(inst->arg.lmem.p)) goto invalid_argument_error;
			if (inst->arg.lmem.typ != 6) goto invalid_argument_error;
			if (inst->arg.lmem.zero != 0) goto invalid_argument_error;
			if (!IS_VALID_REGISTER_ID(inst->arg.lmem.r)) goto invalid_argument_error;
			if (inst->arg.lmem.bit != 0x20) goto invalid_argument_error;
			break;
		case SMEM:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.smem.r);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.smem.bit);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.smem.p);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.smem.typ);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.smem.zero);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_REGISTER_ID(inst->arg.smem.r)) goto invalid_argument_error;
			if (inst->arg.smem.bit != 0x20) goto invalid_argument_error;
			if (!IS_VALID_PREGISTER_ID(inst->arg.smem.p)) goto invalid_argument_error;
			if (inst->arg.smem.zero != 0) goto invalid_argument_error;
			if (inst->arg.smem.typ != 6) goto invalid_argument_error;
			break;
		case PADD:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.padd.p1);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.padd.typ);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.padd.r);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.padd.bit);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.padd.p0);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_PREGISTER_ID(inst->arg.padd.p1)) goto invalid_argument_error;
			if (inst->arg.padd.typ != 6) goto invalid_argument_error;
			if (!IS_VALID_REGISTER_ID(inst->arg.padd.r)) goto invalid_argument_error;
			if (inst->arg.padd.bit != 0x20) goto invalid_argument_error;
			if (!IS_VALID_PREGISTER_ID(inst->arg.padd.p0)) goto invalid_argument_error;
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
		case PCP:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.pcp.p1);
			if (ret == 0) goto fetch_b32code_error;
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.pcp.p0);
			if (ret == 0) goto fetch_b32code_error;

			if (!IS_VALID_PREGISTER_ID(inst->arg.pcp.p1)) goto invalid_argument_error;
			if (!IS_VALID_PREGISTER_ID(inst->arg.pcp.p0)) goto invalid_argument_error;
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
		case DATA:
			{
				int i;
				int dummy;
				inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.data.typ);
				if (ret == 0) goto fetch_b32code_error;
				inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.data.len);
				if (ret == 0) goto fetch_b32code_error;

				if (inst->arg.data.typ != 6) goto invalid_argument_error;

				inst->arg.data.codepos = base+inc;

				// あとで読み込むのでここでは読み飛ばす
				inc += inst->arg.data.len*4;
				if (base+inc > len) goto fetch_b32code_error;
			}
			break;
		case REM:
			inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.uimm);
			if (ret == 0) goto fetch_b32code_error;
			switch (inst->arg.rem.uimm)
			{
				case 0x00:
					inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.rem0.arg1);
					if (ret == 0) goto fetch_b32code_error;
					break;
				case 0x01:
					inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.rem1.arg1);
					if (ret == 0) goto fetch_b32code_error;
					break;
				case 0x02:
					inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.rem2.arg1);
					if (ret == 0) goto fetch_b32code_error;
					inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.rem2.arg2);
					if (ret == 0) goto fetch_b32code_error;
					break;
				case 0x03:
					inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.rem3.arg1);
					if (ret == 0) goto fetch_b32code_error;
					break;
				case 0x34:
					inc += ret = fetch_b32code(code, base+inc, len, &inst->arg.rem.rem34.arg1);
					if (ret == 0) goto fetch_b32code_error;
					break;
				default:
					*error = ERROR_INVALID_INSTRUCTION;
					return 0;
			}
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

int compare_labels(const struct Label* a, const struct Label* b) { return a->id > b->id; }
int prepare_labels(struct Osecpu* osecpu, const unsigned char* code)
{
	int i, j, k;
	int labelcnt;
	struct Label* labels;

	if (osecpu->codelen == 0) return 0;

	for (labelcnt = i = 0; i < osecpu->codelen; i++) {
		if (osecpu->code[i].id == LB) {
			labelcnt++;
		}
	}

	labels = (struct Label*)malloc(sizeof(struct Label) * labelcnt);
	if (!labels) return 0;

	for (i = j = 0; j < labelcnt; i++) {
		if (osecpu->code[i].id != LB) continue;
		labels[j].id = osecpu->code[i].arg.lb.uimm;
		labels[j].pos = i;
		labels[j].data = NULL;
		if (i+1 <= osecpu->codelen && osecpu->code[i+1].id == DATA) {
			// store data if next instruction is data
			labels[j].datalen = osecpu->code[i+1].arg.data.len;
			labels[j].data = (int*)malloc(labels[j].datalen * sizeof(int));
			if (!labels[j].data) return 0;
			for (k = 0; k < labels[j].datalen; k++) {
				int* base = (int*)((unsigned char*)code + osecpu->code[i+1].arg.data.codepos);
				labels[j].data[k] = BIG_TO_LITTLE(base[k]);
			}
		}
		j++;
	}

	qsort(labels, labelcnt, sizeof(struct Label), (int (*)(const void*, const void*))compare_labels);

	osecpu->labels = labels;
	osecpu->labelcnt = labelcnt;

	return 1;
}

int prepare_code(struct Osecpu* osecpu, const uint8_t* code, const int len)
{
	int error;
	const int instcnt = count_instructions(code, len, &error);
	int i;
	int codepos;

	if (error != 0) {
		osecpu->error = error;
		return 0;
	}

	osecpu->code = (struct Instruction*)malloc(sizeof(struct Instruction) * instcnt);
	if (!osecpu->code) return 0;

	for (codepos = i = 0; i < instcnt; i++) {
		// error always must be 0
		// (count_instructions() already validated arguments)
		codepos += fetch_b32instruction(code, codepos, len, &osecpu->code[i], &error);
	}

	osecpu->codelen = instcnt;

	if (!prepare_labels(osecpu, code)) return 0;

	return 1;
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

	// Validate a signature
	if (strncmp(code, B32_SIGNATURE, strlen(B32_SIGNATURE)) != 0) {
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

	prepare_code(osecpu, code, len);
	if (!osecpu->code) return -1;

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
				, i+16*0, osecpu->pregisters[i+16*0].p.code
				, i+16*1, osecpu->pregisters[i+16*1].p.code
				, i+16*2, osecpu->pregisters[i+16*2].p.code
				, i+16*3, osecpu->pregisters[i+16*3].p.code
			  );
		i++;
	}
	printf("\n");

	printf("Registers(debug):\n");
	for (i = 0; i < 4; i++) {
		printf("DR%X: %08X\t", i, osecpu->dregisters[i]);
	}
	printf("\n\n");

	printf("Labels:\n");
	for (i = 0; i < osecpu->labelcnt; i++) {
		printf("%06x: %08x", osecpu->labels[i].id, osecpu->labels[i].pos);
		if (osecpu->labels[i].data) {
			printf(" (data)\n");
			printf("          |00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
			printf("  --------+-----------------------------------------------");
			for (j = 0; j < osecpu->labels[i].datalen; j++) {
				if (j == 0 || j%16 == 0) {
					printf("\n  %08x|", j);
				}
				printf("%02x ", osecpu->labels[i].data[j]);
			}
		}
		printf("\n");
	}
	printf("\n");
}

void do_operate_instruction(struct Osecpu* osecpu, const struct Instruction* inst)
{
	int* r0 = &osecpu->registers[inst->arg.operate.r0];
	const int r1 = osecpu->registers[inst->arg.operate.r1];
	const int r2 = osecpu->registers[inst->arg.operate.r2];
	switch (inst->id) {
		case OR:  *r0 = r1 |  r2; break;
		case XOR: *r0 = r1 ^  r2; break;
		case AND: *r0 = r1 &  r2; break;
		case ADD: *r0 = r1 +  r2; break;
		case SUB: *r0 = r1 -  r2; break;
		case MUL: *r0 = r1 *  r2; break;
		case SHL: *r0 = r1 << r2; break;
		case SAR: *r0 = r1 >> r2; break;
		case DIV:
		case MOD:
			if (r2 == 0) {
				osecpu->error = ERROR_DIVISION_BY_ZERO;
			} else if (inst->id == DIV) {
				*r0 = r1 / r2;
			} else if (inst->id == MOD) {
				*r0 = r1 % r2;
			}
			break;
	}
}

void do_compare_instruction(struct Osecpu* osecpu, const struct Instruction* inst)
{
	int* r0 = &osecpu->registers[inst->arg.compare.r0];
	const int r1 = osecpu->registers[inst->arg.compare.r1];
	const int r2 = osecpu->registers[inst->arg.compare.r2];
	switch(inst->id) {
		case CMPE:  *r0 = (r1 == r2) ? -1 : 0; break;
		case CMPNE: *r0 = (r1 != r2) ? -1 : 0; break;
		case CMPL:  *r0 = (r1 <  r2) ? -1 : 0; break;
		case CMPGE: *r0 = (r1 >= r2) ? -1 : 0; break;
		case CMPLE: *r0 = (r1 <= r2) ? -1 : 0; break;
		case CMPG:  *r0 = (r1 >  r2) ? -1 : 0; break;
	}
}

int compare_label_bsearch(const int* a, const struct Label* b) { return *a - b->id; }
const struct Label* get_label(struct Osecpu* osecpu, int id)
{
	struct Label* result;
	result = (struct Label*)bsearch(&id, osecpu->labels,
			osecpu->labelcnt, sizeof(struct Label),
			(int (*)(const void*, const void*))compare_label_bsearch
			);
	return result;
}

void do_instruction(struct Osecpu* osecpu, const struct Instruction* inst)
{
	switch (inst->id) {
		case NOP:
		case LB:
		case REM:
		case DATA:
			// Nothing to do
			break;
		case LIMM:
			osecpu->registers[inst->arg.limm.r] = inst->arg.limm.imm;
			break;
		case PLIMM:
			{
				const struct Label* label = get_label(osecpu, inst->arg.plimm.uimm);
				if (label) {
					if (label->data) {
						osecpu->pregisters[inst->arg.plimm.p].type = SINT32;
						osecpu->pregisters[inst->arg.plimm.p].p.sint32 = label->data;
					} else {
						osecpu->pregisters[inst->arg.plimm.p].type = CODE;
						osecpu->pregisters[inst->arg.plimm.p].p.code = label->pos;
					}
				} else {
					osecpu->error = ERROR_LABEL_DOES_NOT_EXIST;
				}
			}
			break;
		case CND:
			if (!(osecpu->registers[inst->arg.cnd.r] & 1)) {
				// Skip a next instruction
				osecpu->pregisters[0x3f].p.code++;
			}
			break;
		case LMEM:
			{
				const struct OsecpuPointer* p = &osecpu->pregisters[inst->arg.lmem.p];
				if (p->type == SINT32) {
					osecpu->registers[inst->arg.lmem.r] = *p->p.sint32;
				} else {
					osecpu->error = ERROR_INVALID_LABEL_TYPE;
				}
			}
			break;
		case SMEM:
			{
				struct OsecpuPointer* p = &osecpu->pregisters[inst->arg.lmem.p];
				if (p->type == SINT32) {
					*p->p.sint32 = osecpu->registers[inst->arg.lmem.r];
				} else {
					osecpu->error = ERROR_INVALID_LABEL_TYPE;
				}
			}
			break;
		case PADD:
			// TODO: オーバーフロー確認
			{
				struct OsecpuPointer* p0 = &osecpu->pregisters[inst->arg.padd.p0];
				const struct OsecpuPointer* p1 = &osecpu->pregisters[inst->arg.padd.p1];
				const int r = osecpu->registers[inst->arg.padd.r];
				switch (p1->type) {
					case CODE: *p0 = *p1; p0->p.code++; break;
					case SINT32: *p0 = *p1; p0->p.sint32++; break;
					default: osecpu->error = ERROR_INVALID_LABEL_TYPE; break;
				}
			}
			break;
		case LIDR:
			osecpu->dregisters[inst->arg.lidr.dr] = inst->arg.lidr.imm;
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
			do_operate_instruction(osecpu, inst);
			break;
		case PCP:
			osecpu->pregisters[inst->arg.pcp.p0] = osecpu->pregisters[inst->arg.pcp.p1];
			break;
		case CMPE:
		case CMPNE:
		case CMPL:
		case CMPGE:
		case CMPLE:
		case CMPG:
			do_compare_instruction(osecpu, inst);
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

struct Instruction* fetch_instruction(struct Osecpu* osecpu)
{
	const int pc = osecpu->pregisters[0x3f].p.code;
	if (pc == LABEL_API) return NULL;
	if (osecpu->pregisters[0x3f].p.code+1 > osecpu->codelen) return 0;
	osecpu->pregisters[0x3f].p.code++;
	return &osecpu->code[pc];
}

void initialize_osecpu(struct Osecpu* osecpu)
{
	int i, j;

	// initialize P01〜P04
	for (i = j = 0; i < osecpu->labelcnt; i++) {
		if (osecpu->labels[i].data) {
			j++;
			osecpu->pregisters[j].type = SINT32;
			osecpu->pregisters[j].p.sint32 = osecpu->labels[i].data;
			if (j >= 4) break; 
		}
	}
}

int run_b32(struct Osecpu* osecpu)
{
	struct Instruction* inst;

	// TODO: ブレーク機能とかはまだついてないのでとりあえずここで初期化する
	initialize_osecpu(osecpu);

	while (1) {
		inst = fetch_instruction(osecpu);
		if (!inst) {
			if (osecpu->pregisters[0x3f].p.code != LABEL_API) break;
			call_api(osecpu);
		} else {
			do_instruction(osecpu, inst);
		}
		if (osecpu->error != 0) return -1;
	}
	return 0;
}

