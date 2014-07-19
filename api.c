#include "api.h"

#include <stdio.h>

void call_api(struct Osecpu* osecpu)
{
	switch (osecpu->registers[0x30])
	{
		default:
			osecpu->error = ERROR_NOT_IMPLEMENTED_API;
			break;
	}
	osecpu->pregisters[0x3f] = osecpu->pregisters[0x30];
}

