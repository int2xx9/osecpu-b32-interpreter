#include <cutter.h>
#include "../osecpu.h"

#define R00 0
#define R01 1
#define R02 2

#define P00 0
#define P01 1
#define P3F 0x3f

#define DR0 0

#define little_to_big(val) \
	(unsigned)((val)&0xff000000) >> 24, \
	(unsigned)((val)&0x00ff0000) >> 16, \
	(unsigned)((val)&0x0000ff00) >>  8, \
	(unsigned)((val)&0x000000ff) >>  0

#define NOP() \
	0x76, 0x00, 0x00, 0x00

#define LB(opt, uimm) \
	0x76, 0x00, 0x00, 0x01, \
	little_to_big(uimm | 0x76000000), \
	little_to_big(opt | 0x76000000)

#define LIMM(bit, r, val) \
	0x76, 0x00, 0x00, 0x02, \
	0xff, 0xff, 0xf7, 0x88, \
	little_to_big(val), \
	0x76, 0x00, 0x00, (char)r, \
	0x76, 0x00, 0x00, (char)bit

#define PLIMM(p, val) \
	0x76, 0x00, 0x00, 0x03, \
	little_to_big(val | 0x76000000), \
	little_to_big(p | 0x76000000)

#define CND(r) \
	0x76, 0x00, 0x00, 0x04, \
	little_to_big(r | 0x76000000)

#define LIDR(r, val) \
	0x76, 0x00, 0x00, 0xfd, \
	0xff, 0xff, 0xf7, 0x88, \
	little_to_big(val), \
	0x76, 0x00, 0x00, (char)r

#define OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define OR(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x10, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define XOR(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x11, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define AND(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x12, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define ADD(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x14, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define SUB(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x15, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define MUL(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x16, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define SHL(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x18, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define SAR(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x19, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define DIV(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x1a, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define MOD(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x1b, \
	OPERATE_INSTRUCTION_ARGUMENTS(bit, r0, r1, r2)

#define PCP(p0, p1) \
	0x76, 0x00, 0x00, 0x1e, \
	little_to_big(p1 | 0x76000000), \
	little_to_big(p0 | 0x76000000)

#define COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)bit1, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit0

#define CMPE(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x20, \
	COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2)

#define CMPNE(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x21, \
	COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2)

#define CMPL(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x22, \
	COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2)

#define CMPGE(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x23, \
	COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2)

#define CMPLE(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x24, \
	COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2)

#define CMPG(bit0, bit1, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x25, \
	COMPARE_INSTRUCTION_ARGUMENTS(bit0, bit1, r0, r1, r2)

#define REM(uimm, len) \
	0x76, 0x00, 0x00, 0xfe, \
	little_to_big(uimm | 0x76000000), \
	little_to_big(len | 0x76000000)

struct Osecpu* run_code(uint8_t code[], int len)
{
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, len);
	run_b32(osecpu);
	return osecpu;
}

void test_instruction_nop()
{
	char code[] = {
		NOP()
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(1, osecpu->pregisters[0x3f]);
	free_osecpu(osecpu);
}

void test_instruction_lb()
{
	char code[] = {
		LB(0, 1),
		LB(0, 0),
		LIMM(32, R00, 0),
		LB(0, 2)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(3, osecpu->labelcnt);
	cut_assert_equal_int(0, osecpu->labels[0].id);
	cut_assert_equal_int(1, osecpu->labels[0].pos);
	cut_assert_equal_int(1, osecpu->labels[1].id);
	cut_assert_equal_int(0, osecpu->labels[1].pos);
	cut_assert_equal_int(2, osecpu->labels[2].id);
	cut_assert_equal_int(3, osecpu->labels[2].pos);
	free_osecpu(osecpu);
}

void test_instruction_limm()
{
	char code[] = {
		LIMM(32, R00, 0x12345678)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x12345678, osecpu->registers[0]);
	free_osecpu(osecpu);
}

void test_instruction_plimm()
{
	char code[] = {
		PLIMM(P3F, 0),
		LIMM(32, R00, 0xffffffff),
		LB(0, 0)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0, osecpu->registers[0]);
	free_osecpu(osecpu);
}

void test_instruction_cnd()
{
	char code[] = {
		LIMM(32, R00, -1),
		CND(R00),
		LIMM(32, R01, 0x12345678),
		LIMM(32, R00, 2),
		CND(R00),
		LIMM(32, R02, 0x12345678)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x12345678, osecpu->registers[1]);
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_lidr()
{
	char code[] = {
		LIDR(DR0, 0x12345678)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x12345678, osecpu->dregisters[0]);
	free_osecpu(osecpu);
}

void test_instruction_or()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x11001100),
		OR(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010|0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_xor()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x11001100),
		XOR(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010^0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_and()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x11001100),
		AND(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010&0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_add()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x11001100),
		ADD(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010+0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_sub()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x11001100),
		SUB(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010-0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_mul()
{
	char code[] = {
		LIMM(32, R00, 0x00001010),
		LIMM(32, R01, 0x00001100),
		MUL(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x00001010*0x00001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_shl()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000010),
		SHL(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010<<0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_sar()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000010),
		SAR(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0x10101010>>0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_div()
{
	char code1[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000010),
		DIV(32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000000),
		DIV(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(0x10101010/0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);

	// division_by_zero_error
	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(ERROR_DIVISION_BY_ZERO, osecpu->error);
	free_osecpu(osecpu);
}

void test_instruction_mod()
{
	char code1[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000010),
		MOD(32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000000),
		MOD(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(0x10101010%0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);

	// division_by_zero_error
	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(ERROR_DIVISION_BY_ZERO, osecpu->error);
	free_osecpu(osecpu);
}

void test_instruction_pcp()
{
	char code[] = {
		PLIMM(P00, 0),
		PCP(P01, P00),
		LB(0, 0)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(2, osecpu->pregisters[1]);
	free_osecpu(osecpu);
}

void test_instruction_cmpe()
{
	char code1[] = {
		LIMM(32, R00, 0x00000000),
		LIMM(32, R01, 0x00000000),
		CMPE(32, 32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x00000000),
		LIMM(32, R01, 0x12345678),
		CMPE(32, 32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_cmpne()
{
	char code1[] = {
		LIMM(32, R00, 0x00000000),
		LIMM(32, R01, 0x00000000),
		CMPNE(32, 32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x00000000),
		LIMM(32, R01, 0x12345678),
		CMPNE(32, 32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_cmpl()
{
	char code1[] = {
		LIMM(32, R00, 0x00010000),
		LIMM(32, R01, 0x00010000),
		CMPL(32, 32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00000fff),
		CMPL(32, 32, R02, R00, R01)
	};
	char code3[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00001001),
		CMPL(32, 32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code3, sizeof(code3));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_cmpge()
{
	char code1[] = {
		LIMM(32, R00, 0x00010000),
		LIMM(32, R01, 0x00010000),
		CMPGE(32, 32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00000fff),
		CMPGE(32, 32, R02, R00, R01)
	};
	char code3[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00001001),
		CMPGE(32, 32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code3, sizeof(code3));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_cmple()
{
	char code1[] = {
		LIMM(32, R00, 0x00010000),
		LIMM(32, R01, 0x00010000),
		CMPLE(32, 32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00000fff),
		CMPLE(32, 32, R02, R00, R01)
	};
	char code3[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00001001),
		CMPLE(32, 32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code3, sizeof(code3));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_cmpg()
{
	char code1[] = {
		LIMM(32, R00, 0x00010000),
		LIMM(32, R01, 0x00010000),
		CMPG(32, 32, R02, R00, R01)
	};
	char code2[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00000fff),
		CMPG(32, 32, R02, R00, R01)
	};
	char code3[] = {
		LIMM(32, R00, 0x00001000),
		LIMM(32, R01, 0x00001001),
		CMPG(32, 32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code1, sizeof(code1));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code2, sizeof(code2));
	cut_assert_equal_int(-1, osecpu->registers[2]);
	free_osecpu(osecpu);

	osecpu = run_code(code3, sizeof(code3));
	cut_assert_equal_int(0, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_rem()
{
	char code[] = {
		REM(0, 0)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(1, osecpu->pregisters[0x3f]);
	free_osecpu(osecpu);
}

extern int fetch_b32code(const uint8_t*, const int, const int, int*);
void test_fetch_b32code()
{
	uint8_t code1[] = {0x76, 0x12, 0x34, 0x56};
	uint8_t code2[] = {0xff, 0xff, 0xf7, 0x88, 0x12, 0x34, 0x56, 0x78};
	uint8_t code3[] = {0x00};
	uint8_t code4[] = {0xff, 0xff, 0xf7, 0x88};
	int ret;
	int code_ret;

	code_ret = fetch_b32code(code1, 0, sizeof(code1), &ret);
	cut_assert_equal_int(0x123456, ret);
	cut_assert_equal_int(4, code_ret);

	code_ret = fetch_b32code(code2, 0, sizeof(code2), &ret);
	cut_assert_equal_int(0x12345678, ret);
	cut_assert_equal_int(8, code_ret);

	code_ret = fetch_b32code(code3, 0, sizeof(code3), &ret);
	cut_assert_equal_int(0, code_ret);

	code_ret = fetch_b32code(code4, 0, sizeof(code4), &ret);
	cut_assert_equal_int(0, code_ret);
}

extern int fetch_b32instruction(const uint8_t*, const int, const int, struct Instruction*, int*);
void test_fetch_b32instruction()
{
	uint8_t code_limm[] = {LIMM(32, R00, 0x12345678)};
	uint8_t code_lidr[] = {LIDR(DR0, 0x12345678)};
	uint8_t code_operate[] = {OR(32, R02, R00, R01)};
	uint8_t code_compare[] = {CMPE(32, 32, R02, R00, R01)};
	uint8_t code_eoc[] = {};
	uint8_t code_unexpected_eoc[] = {0x76, 0x00, 0x00, 0x10};
	struct Instruction inst;
	int error;
	int ret;

	ret = fetch_b32instruction(code_limm, 0, sizeof(code_limm), &inst, &error);
	cut_assert_equal_int(4*5, ret);
	cut_assert_equal_int(0, error);

	ret = fetch_b32instruction(code_lidr, 0, sizeof(code_lidr), &inst, &error);
	cut_assert_equal_int(4*4, ret);
	cut_assert_equal_int(0, error);

	ret = fetch_b32instruction(code_operate, 0, sizeof(code_operate), &inst, &error);
	cut_assert_equal_int(4*5, ret);
	cut_assert_equal_int(0, error);

	ret = fetch_b32instruction(code_compare, 0, sizeof(code_compare), &inst, &error);
	cut_assert_equal_int(4*6, ret);
	cut_assert_equal_int(0, error);

	ret = fetch_b32instruction(code_eoc, 0, sizeof(code_eoc), &inst, &error);
	cut_assert_equal_int(0, ret);
	cut_assert_equal_int(0, error);

	ret = fetch_b32instruction(code_unexpected_eoc, 0, sizeof(code_unexpected_eoc), &inst, &error);
	cut_assert_equal_int(0, ret);
	cut_assert_equal_int(ERROR_UNEXPECTED_EOC, error);
}

extern int count_instructions(const uint8_t*, const int, int*);
void test_count_instructions()
{
	uint8_t code_empty[] = {};
	uint8_t code_1[] = {
		LIMM(32, R00, 0x12345678)
	};
	int error;
	int ret;

	ret = count_instructions(code_empty, sizeof(code_empty), &error);
	cut_assert_equal_int(0, ret);
	cut_assert_equal_int(0, error);

	ret = count_instructions(code_1, sizeof(code_1), &error);
	cut_assert_equal_int(1, ret);
	cut_assert_equal_int(0, error);
}

extern const struct Label* get_label(struct Osecpu*, int);
void test_get_label()
{
	char code[] = {
		LB(0, 1),
		LB(0, 0),
		LB(0, 2)
	};
	struct Osecpu* osecpu;
	osecpu = run_code(code, sizeof(code));
	cut_assert_equal_int(0, get_label(osecpu, 0)->id);
	cut_assert_equal_int(1, get_label(osecpu, 1)->id);
	cut_assert_equal_int(2, get_label(osecpu, 2)->id);
	cut_assert_null(get_label(osecpu, 3));
	free_osecpu(osecpu);
}

