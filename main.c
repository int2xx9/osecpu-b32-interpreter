#include "osecpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	struct Osecpu* osecpu;

	if (argc < 2) {
		printf("Usage: %s app.b32\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	osecpu = init_osecpu();
	if (!osecpu) {
		printf("init_osecpu() failed\n");
		exit(EXIT_FAILURE);
	}

	if (load_b32_from_file(osecpu, argv[1]) == -1) {
		printf("load_b32() error\n");
		if (osecpu->code) free(osecpu->code);
		free(osecpu);
		exit(EXIT_FAILURE);
	}

	run_b32(osecpu);
	switch (osecpu->error) {
		case ERROR_INVALID_INSTRUCTION:
			fprintf(stderr, "Error: invalid instruction error\n");
			break;
		case ERROR_INVALID_ARGUMENT:
			fprintf(stderr, "Error: invalid argument error\n");
			break;
		case ERROR_DIVISION_BY_ZERO:
			fprintf(stderr, "Error: division by zero error\n");
			break;
	}

	coredump(osecpu);

	free_osecpu(osecpu);
	return 0;
}

