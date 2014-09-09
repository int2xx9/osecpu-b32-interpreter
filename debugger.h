#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include "osecpu.h"
#include <pthread.h>

typedef struct _OsecpuDebugger
{
	struct Osecpu* osecpu;
	// XXX: a pointer to Gtk::Window
	// This header is loaded by C sources but C cannot understand gtkmm.h
	void* window;
	pthread_t windowThread;
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

