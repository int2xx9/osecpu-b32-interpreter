#ifndef _REVERSE_ASKA_H_
#define _REVERSE_ASKA_H_

#include "osecpu.h"

typedef struct _ReverseAska
{
	uint8_t* code;
	int codelen;
	int* instruction_index;
	int idxcnt;
} ReverseAska;

/*
// TODO: free once
struct ReverseAskaInstruction
{
	// inst_raw refers data + 0
	// inst_str refers data + strlen(inst_raw) + 1
	int number;
	char* inst_raw;
	char* inst_str;
	char data[0];
};
*/

struct ReverseAskaInstruction
{
	int number;
	char* inst_str;
};

ReverseAska* reverse_aska_init(const uint8_t* code, int codelen);
void reverse_aska_free(ReverseAska*);
struct ReverseAskaInstruction* get_instruction_string(ReverseAska*, int);

#endif

