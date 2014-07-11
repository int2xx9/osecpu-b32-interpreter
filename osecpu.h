#ifndef _OSECPU_H_
#define _OSECPU_H_

#include <inttypes.h>

struct Osecpu
{
	int registers[0x40];
	int pregisters[0x40];
	int dregisters[4];
	uint8_t* code;
	long codelen;

	int invalid_instruction_error;
	int invalid_argument_error;
	int division_by_zero_error;
};

struct Osecpu* init_osecpu();
void free_osecpu(struct Osecpu*);
int load_b32_from_file(struct Osecpu*, const char*);
int load_b32_from_memory(struct Osecpu*, const uint8_t*, long);
void coredump(struct Osecpu*);
int run_b32(struct Osecpu*);

#endif

