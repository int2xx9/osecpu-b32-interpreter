#include <cutter.h>
#include "../osecpu.h"

void test_instruction_limm()
{
	char code[] =
		// LIMM(32, R00, 0x12345678);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x12\x34\x56\x78"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x12345678, osecpu->registers[0]);
	free_osecpu(osecpu);
}

void test_instruction_lidr()
{
	char code[] =
		// LIDR(DR0, 0x12345678);
		"\x76\x00\x00\xfd"
		"\xff\xff\xf7\x88"
		"\x12\x34\x56\x78"
		"\x76\x00\x00\x00"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x12345678, osecpu->dregisters[0]);
	free_osecpu(osecpu);
}

void test_instruction_or()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 0x11001100);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x11\x00\x11\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// OR(32, R00, R01, R02);
		"\x76\x00\x00\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010|0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_xor()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 0x11001100);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x11\x00\x11\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// XOR(32, R00, R01, R02);
		"\x76\x00\x00\x11"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010^0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_and()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 0x11001100);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x11\x00\x11\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// AND(32, R02, R00, R01);
		"\x76\x00\x00\x12"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010&0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_add()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 0x11001100);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x11\x00\x11\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// ADD(32, R02, R00, R01);
		"\x76\x00\x00\x14"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010+0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_sub()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 0x11001100);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x11\x00\x11\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// SUB(32, R02, R00, R01);
		"\x76\x00\x00\x15"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010-0x11001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_mul()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x00\x00\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 0x11001100);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x00\x00\x11\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// SUB(32, R02, R00, R01);
		"\x76\x00\x00\x16"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x00001010*0x00001100, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_shl()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 16);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x00\x00\x00\x10"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// SHL(32, R02, R00, R01);
		"\x76\x00\x00\x18"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010<<0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_sar()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 16);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x00\x00\x00\x10"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// SAR(32, R02, R00, R01);
		"\x76\x00\x00\x19"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010>>0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);
}

void test_instruction_div()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 16);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x00\x00\x00\x10"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// DIV(32, R02, R00, R01);
		"\x76\x00\x00\x1a"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010/0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);

	// division_by_zero_error
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	osecpu->code[4*7+3] = 0;
	run_b32(osecpu);
	cut_assert_not_equal_int(0, osecpu->division_by_zero_error);
	free_osecpu(osecpu);
}

void test_instruction_mod()
{
	char code[] =
		// LIMM(32, R00, 0x10101010);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x10\x10\x10\x10"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x20"
		// LIMM(32, R01, 16);
		"\x76\x00\x00\x02"
		"\xff\xff\xf7\x88"
		"\x00\x00\x00\x10"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x20"
		// MOD(32, R02, R00, R01);
		"\x76\x00\x00\x1b"
		"\x76\x00\x00\x00"
		"\x76\x00\x00\x01"
		"\x76\x00\x00\x02"
		"\x76\x00\x00\x20"
		;
	struct Osecpu* osecpu;
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	run_b32(osecpu);
	cut_assert_equal_int(0x10101010%0x00000010, osecpu->registers[2]);
	free_osecpu(osecpu);

	// division_by_zero_error
	osecpu = init_osecpu();
	load_b32_from_memory(osecpu, code, sizeof(code)-1);
	osecpu->code[4*7+3] = 0;
	run_b32(osecpu);
	cut_assert_not_equal_int(0, osecpu->division_by_zero_error);
	free_osecpu(osecpu);
}

