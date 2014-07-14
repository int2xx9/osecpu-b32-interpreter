#include <cutter.h>
#include "../osecpu.h"

#define R00 0
#define R01 1
#define R02 2

#define DR0 0

#define little_to_big(val) \
	(unsigned)((val)&0xff000000) >> 24, \
	(unsigned)((val)&0x00ff0000) >> 16, \
	(unsigned)((val)&0x0000ff00) >>  8, \
	(unsigned)((val)&0x000000ff) >>  0

#define LIMM(bit, r, val) \
	0x76, 0x00, 0x00, 0x02, \
	0xff, 0xff, 0xf7, 0x88, \
	little_to_big(val), \
	0x76, 0x00, 0x00, (char)r, \
	0x76, 0x00, 0x00, (char)bit

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

struct Osecpu* run_code(uint8_t code[], int len)
{
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, len);
	run_b32(osecpu);
	return osecpu;
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

extern int fetch_b32code(const uint8_t*, int*);
void test_fetch_b32code()
{
	uint8_t code1[] = {0x76, 0x12, 0x34, 0x56};
	uint8_t code2[] = {0xff, 0xff, 0xf7, 0x88, 0x12, 0x34, 0x56, 0x78};
	uint8_t code3[] = {0x00};
	int ret;
	int code_ret;

	code_ret = fetch_b32code(code1, &ret);
	cut_assert_equal_int(0x123456, ret);
	cut_assert_equal_int(4, code_ret);

	code_ret = fetch_b32code(code2, &ret);
	cut_assert_equal_int(0x12345678, ret);
	cut_assert_equal_int(8, code_ret);

	code_ret = fetch_b32code(code3, &ret);
	cut_assert_equal_int(0, code_ret);
}

