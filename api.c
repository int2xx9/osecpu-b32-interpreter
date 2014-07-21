#include "api.h"
#include "window.h"

#include <stdio.h>

#define API_OPENWIN		0x0010
#define API_DRAWPOINT	0x0002

void api0010_openWin(struct Osecpu* osecpu)
{
	const int width = osecpu->registers[0x33];
	const int height = osecpu->registers[0x34];

	// TODO: 既に初期化されていないかチェック
	osecpu->window = window_create(width, height);
}

void api0002_drawPoint(struct Osecpu* osecpu)
{
	const int mode = osecpu->registers[0x31];
	const int x = osecpu->registers[0x33];
	const int y = osecpu->registers[0x34];
	const int color = osecpu->registers[0x32];
	window_draw_point(osecpu->window, color, x, y);
}

void call_api(struct Osecpu* osecpu)
{
	switch (osecpu->registers[0x30])
	{
		case API_OPENWIN:
			api0010_openWin(osecpu);
			break;
		case API_DRAWPOINT:
			api0002_drawPoint(osecpu);
			break;
		default:
			osecpu->error = ERROR_NOT_IMPLEMENTED_API;
			break;
	}
	osecpu->pregisters[0x3f] = osecpu->pregisters[0x30];
}

