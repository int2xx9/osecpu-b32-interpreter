#ifndef _OSECPU_H_
#define _OSECPU_H_

#include "debugger.h"

#include <inttypes.h>
#include <setjmp.h>

#define ERROR_INVALID_INSTRUCTION	1
#define ERROR_INVALID_ARGUMENT		2
#define ERROR_DIVISION_BY_ZERO		3
#define ERROR_INVALID_B32_CODE		4
#define ERROR_UNEXPECTED_EOC		5
#define ERROR_LABEL_DOES_NOT_EXIST	6
#define ERROR_NOT_IMPLEMENTED_API	7
#define ERROR_INVALID_MODE			8
#define ERROR_INVALID_COLOR			9
#define ERROR_INVALID_LABEL_TYPE	10
#define ERROR_MALLOC				11

static const char* ErrorMessages[] = {
	"exited successfully",
	// ERROR_INVALID_INSTRUCTION
	"invalid Instruction error",
	// ERROR_INVALID_ARGUMENT
	"invalid argument error",
	// ERROR_DIVISION_BY_ZERO
	"division by zero error",
	// ERROR_INVALID_B32_CODE
	"invalid b32 code error",
	// ERROR_UNEXPECTED_EOC
	"unexpected end of code",
	// ERROR_LABEL_DOES_NOT_EXIST
	"a label doesn't exist",
	// ERROR_NOT_IMPLEMENTED_API
	"not implemented api",
	// ERROR_INVALID_MODE
	"invalid mode",
	// ERROR_INVALID_COLOR
	"invalid color",
	// ERROR_INVALID_LABEL_TYPE
	"invalid label type",
	// ERROR_MALLOC
	"an error occured on malloc()",
};

#define IS_VALID_REGISTER_ID(regid) ((regid) >= 0 && (regid) <= 0x3f)
#define IS_VALID_PREGISTER_ID(regid) ((regid) >= 0 && (regid) <= 0x3f)
#define IS_VALID_DREGISTER_ID(regid) ((regid) >= 0 && (regid) <= 4)

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
	DATA	= 0x2e,
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
			int uimm;
			int opt;
		} lb;
		struct
		{
			int imm;
			int r;
			int bit;
		} limm;
		struct
		{
			int uimm;
			int p;
		} plimm;
		struct
		{
			int r;
		} cnd;
		struct
		{
			int bit;
			int r;
			int typ;
			int p;
			int zero;
		} lmem;
		struct
		{
			int bit;
			int r;
			int typ;
			int p;
			int zero;
		} smem;
		struct
		{
			int bit;
			int p0;
			int typ;
			int p1;
			int r;
		} padd;
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
			int p0;
			int p1;
		} pcp;
		struct
		{
			int r1;
			int r2;
			int bit1;
			int r0;
			int bit0;
		} compare;
		struct
		{
			int typ;
			int len;
			int codepos;
		} data;
		union
		{
			int uimm;
			struct
			{
				int arg1;
			} rem0;
			struct
			{
				int arg1;
			} rem1;
			struct
			{
				int arg1;
				int arg2;
			} rem2;
			struct
			{
				int arg1;
			} rem3;
			struct
			{
				int arg1;
			} rem34;
			struct
			{
				int enabled;
				int arg1;
			} rem1ff;
		} rem;
	} arg;
};

struct Label
{
	int id;
	int pos;
	// XXX: DATA関係の変数を別の場所に移動したい (データかどうかのフラグだけ保持したい)
	// XXX: T_UINT8以外への対応
	uint8_t* data;
	int datalen;
};

enum OsecpuPointerType
{
	NOT_INITIAlIZED,
	CODE,
	UINT8,
};

struct OsecpuPointer
{
	enum OsecpuPointerType type;
	union
	{
		uint8_t* uint8;
		int code;
	} p;
};

struct Osecpu
{
	int is_initialized;
	int registers[0x40];
	struct OsecpuPointer pregisters[0x40];
	int dregisters[4];
	struct Instruction* code;
	int codelen;
	struct Label* labels;
	int labelcnt;
	int error;
	jmp_buf abort_to;
	struct OsecpuWindow* window;
};

#ifdef __cplusplus
extern "C" {
#endif
void abort_vm(struct Osecpu*, int, int);
const char* get_error_text(int);
struct Osecpu* init_osecpu();
void free_osecpu(struct Osecpu*);
int load_b32_from_file(struct Osecpu*, const char*);
int load_b32_from_memory(struct Osecpu*, const uint8_t*, long);
void coredump(struct Osecpu*);
void initialize_osecpu(struct Osecpu*);
int do_next_instruction(struct Osecpu*);
int restart_osecpu(struct Osecpu*);
int continue_osecpu(struct Osecpu*);
#ifdef __cplusplus
}
#endif

#endif

