#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include "osecpu.h"

typedef struct _OsecpuDebugger
{
	struct Osecpu* osecpu;
} OsecpuDebugger;

#ifdef __cplusplus
extern "C" {
#endif
OsecpuDebugger* debugger_init(struct Osecpu*);
void debugger_free(OsecpuDebugger*);
void debugger_open(OsecpuDebugger*);
#ifdef __cplusplus
}
#endif

#endif

