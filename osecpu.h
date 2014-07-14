#ifndef _OSECPU_H_
#define _OSECPU_H_

#include <inttypes.h>

#define ERROR_INVALID_INSTRUCTION	1
#define ERROR_INVALID_ARGUMENT		2
#define ERROR_DIVISION_BY_ZERO		3

#define IS_VALID_REGISTER_ID(regid)((regid) >= 0 || (regid) <= 0x3f)
#define IS_VALID_PREGISTER_ID(regid)((regid) >= 0 || (regid) <= 0x3f)
#define IS_VALID_DREGISTER_ID(regid)((regid) >= 0 || (regid) <= 4)

enum InstructionId
{
	NOP		= 0x00,
	LB		= 0x01,
	LIMM	= 0x02,
	PLIMM	= 0x03,
	CND		= 0x04,
	LMEM	= 0x08,
	SMEM	= 0x09,
	PADD	= 0x0e,
	OR		= 0x10,
	XOR		= 0x11,
	AND		= 0x12,
	SBX		= 0x13,
	ADD		= 0x14,
	SUB		= 0x15,
	MUL		= 0x16,
	SHL		= 0x18,
	SAR		= 0x19,
	DIV		= 0x1a,
	MOD		= 0x1b,
	PCP		= 0x1e,
	CMPE	= 0x20,
	CMPNE	= 0x21,
	CMPL	= 0x22,
	CMPGE	= 0x23,
	CMPLE	= 0x24,
	CMPG	= 0x25,
	TSTZ	= 0x26,
	TSTNZ	= 0x27,
	LIDR	= 0xfd,
	REM		= 0xfe,
};

struct Instruction
{
	enum InstructionId id;
	union
	{
		struct
		{
			int imm;
			int r;
			int bit;
		} limm;
		struct
		{
			int imm;
			int dr;
		} lidr;
		struct
		{
			int r1;
			int r2;
			int r0;
			int bit;
		} operate;
		struct
		{
			int r1;
			int r2;
			int bit1;
			int r0;
			int bit0;
		} compare;
	} arg;
};

struct Osecpu
{
	int registers[0x40];
	int pregisters[0x40];
	int dregisters[4];
	uint8_t* code;
	long codelen;
	int error;
};

struct Osecpu* init_osecpu();
void free_osecpu(struct Osecpu*);
int load_b32_from_file(struct Osecpu*, const char*);
int load_b32_from_memory(struct Osecpu*, const uint8_t*, long);
void coredump(struct Osecpu*);
int run_b32(struct Osecpu*);

#endif

