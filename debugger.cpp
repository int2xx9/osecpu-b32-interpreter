#include "debugger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" OsecpuDebugger* debugger_init(struct Osecpu* osecpu)
{
	OsecpuDebugger* debugger;
	debugger = (OsecpuDebugger*)malloc(sizeof(OsecpuDebugger));
	if (!debugger) return NULL;
	memset(debugger, 0, sizeof(OsecpuDebugger));
	debugger->osecpu = osecpu;
	return debugger;
}

extern "C" void debugger_free(OsecpuDebugger* debugger)
{
	free(debugger);
}

extern "C" void debugger_open(OsecpuDebugger* debugger)
{
	char cmdbuf[1024];
	while (1) {
		printf("debug> ");
		fgets(cmdbuf, 1024, stdin);
		cmdbuf[strlen(cmdbuf)-1] = 0;
		if (strcmp(cmdbuf, "continue") == 0) {
			if (continue_osecpu(debugger->osecpu) == 2) {
				printf("breakpoint.\n");
				debugger_open(debugger);
			}
			return;
		} else if (strcmp(cmdbuf, "next") == 0) {
			do_next_instruction(debugger->osecpu);
			if (debugger->osecpu->pregisters[0x3f].p.code >= debugger->osecpu->codelen) {
				return;
			}
		} else if (strcmp(cmdbuf, "coredump") == 0) {
			coredump(debugger->osecpu);
		} else {
			printf("command: continue, next, coredump\n");
		}
	}
}

