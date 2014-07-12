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

#define OR(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x10, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define XOR(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x11, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define AND(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x12, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define ADD(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x14, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define SUB(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x15, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define MUL(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x16, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define SHL(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x18, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define SAR(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x19, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define DIV(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x1a, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

#define MOD(bit, r0, r1, r2) \
	0x76, 0x00, 0x00, 0x1b, \
	0x76, 0x00, 0x00, (char)r1, \
	0x76, 0x00, 0x00, (char)r2, \
	0x76, 0x00, 0x00, (char)r0, \
	0x76, 0x00, 0x00, (char)bit

void test_instruction_limm()
{
	char code[] = {
		LIMM(32, R00, 0x12345678)
	};
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
	cut_assert_equal_int(0x12345678, osecpu->registers[0]);
	free_osecpu(osecpu);
}

void test_instruction_lidr()
{
	char code[] = {
		LIDR(DR0, 0x12345678)
	};
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
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
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010>>0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_div()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000010),
		DIV(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010/0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);

	// division_by_zero_error
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	osecpu->code[4*7+3] = 0;
	run_b32(osecpu);
	cut_assert_not_equal_int(0, osecpu->division_by_zero_error);
	free_osecpu(osecpu);
}

void test_instruction_mod()
{
	char code[] = {
		LIMM(32, R00, 0x10101010),
		LIMM(32, R01, 0x00000010),
		MOD(32, R02, R00, R01)
	};
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010%0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);

	// division_by_zero_error
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code));
	osecpu->code[4*7+3] = 0;
	run_b32(osecpu);
	cut_assert_not_equal_int(0, osecpu->division_by_zero_error);
	free_osecpu(osecpu);
}

