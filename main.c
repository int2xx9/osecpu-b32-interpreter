#include "osecpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct CmdlineArgument
{
	const char* filename;
	int filenamepos;
	int debug_at_startup;
};

void parse_cmdline(int argc, char** argv, struct CmdlineArgument* cmdline)
{
	int i;
	memset(cmdline, 0, sizeof(struct CmdlineArgument));
	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "--debug-at-startup") == 0) {
				cmdline->debug_at_startup = 1;
			}
		} else {
			if (!cmdline->filename) {
				// stop parsing cmdline if argv[i] is filename
				// because cmdlines later than filename are to pass to osecpu applications.
				cmdline->filename = argv[i];
				cmdline->filenamepos = i;
				break;
			}
		}
	}
}

int main(int argc, char** argv)
{
	struct Osecpu* osecpu;
	OsecpuDebugger* debugger;
	struct CmdlineArgument cmdline;
	int osecpu_ret;

	if (argc < 2) {
		printf("Usage: %s [--debug-at-startup] app.b32\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	parse_cmdline(argc, argv, &cmdline);

	osecpu = init_osecpu();
	if (!osecpu) {
		printf("init_osecpu() failed\n");
		exit(EXIT_FAILURE);
	}

	if (load_b32_from_file(osecpu, cmdline.filename) == -1) {
		printf("load_b32() error\n");
		if (osecpu->code) free(osecpu->code);
		free(osecpu);
		exit(EXIT_FAILURE);
	}

	debugger = debugger_init(osecpu);
	if (!debugger) {
		printf("debugger_init() error\n");
		free_osecpu(osecpu);
		exit(EXIT_FAILURE);
	}

	if (cmdline.debug_at_startup) {
		debugger_open(debugger);
	}
	osecpu_ret = restart_osecpu(osecpu);
	if (osecpu_ret == 2) {
		printf("Breakpoint.\n");
		debugger_open(debugger);
	}
	if (osecpu->error) {
		fprintf(stderr, "Error: %s\n", get_error_text(osecpu->error));
	}

	coredump(osecpu);

	debugger_free(debugger);
	free_osecpu(osecpu);
	return 0;
}

