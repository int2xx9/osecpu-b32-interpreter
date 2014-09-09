#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include "osecpu.h"

typedef struct _OsecpuDebugger
{
	struct Osecpu* osecpu;
} OsecpuDebugger;

OsecpuDebugger* debugger_init(struct Osecpu*);
void debugger_free(OsecpuDebugger*);
void debugger_open(OsecpuDebugger*);

#endif

