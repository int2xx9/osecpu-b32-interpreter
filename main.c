#include "osecpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	struct Osecpu* osecpu;

	if (argc < 2) {
		printf("Usage: %s app.ose\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	osecpu = (struct Osecpu*)malloc(sizeof(struct Osecpu));
	if (!osecpu) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memset(osecpu, 0, sizeof(struct Osecpu));

	if (load_b32_from_file(osecpu, argv[1]) == -1) {
		printf("load_b32() error\n");
		if (osecpu->code) free(osecpu->code);
		free(osecpu);
		exit(EXIT_FAILURE);
	}

	run_b32(osecpu);
	if (osecpu->invalid_instruction_error) {
		fprintf(stderr, "Error: invalid instruction error (%08X)\n", osecpu->invalid_instruction_error);
	} else if (osecpu->invalid_argument_error) {
		fprintf(stderr, "Error: invalid argument error\n");
	} else if (osecpu->invalid_argument_error) {
		fprintf(stderr, "Error: division by zero error\n");
	}

	coredump(osecpu);

	if (osecpu->code) free(osecpu->code);
	free(osecpu);

	return 0;
}

