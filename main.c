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

	restart_osecpu(osecpu);
	if (osecpu->error) {
		fprintf(stderr, "Error: %s\n", get_error_text(osecpu->error));
	}

	coredump(osecpu);

	free_osecpu(osecpu);
	return 0;
}

