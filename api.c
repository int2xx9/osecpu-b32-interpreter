#include "api.h"
#include "window.h"

#include <stdio.h>
#include <unistd.h>

#define MODE_COL3	0
#define MODE_COL24	3
#define MODE_XOR	0x08
#define MODE_OR		0x04

#define API_OPENWIN		0x0010
#define API_DRAWPOINT	0x0002
#define API_DRAWLINE	0x0003
#define API_FILLRECT	0x0004
#define API_FILLOVAL	0x0005
#define API_SLEEP		0x0009

const int COL3TBL[] = {
	0x000000, 0xff0000, 0x00ff00,
	0xffff00, 0x0000ff, 0xff00ff,
	0x00ffff, 0xffffff,
};

int get_color(struct Osecpu* osecpu, int mode, int color)
{
	int ret_color;
	switch (mode)
	{
		case MODE_COL3:
			if (color < 0 || color >= sizeof(COL3TBL)/sizeof(COL3TBL[0])) goto error_invalid_color;
			ret_color = COL3TBL[color];
			break;
		case MODE_COL24:
			if (color > 0xff0000) goto error_invalid_color;
			ret_color = color;
			break;
		default:
			osecpu->error = ERROR_INVALID_MODE;
			return -1;
	}
	return ret_color;

error_invalid_color:
	osecpu->error = ERROR_INVALID_COLOR;
	return -1;
}

void api0010_openWin(struct Osecpu* osecpu)
{
	const int width = osecpu->registers[0x33];
	const int height = osecpu->registers[0x34];

	// TODO: 既に初期化されていないかチェック
	osecpu->window = window_create(width, height);
	window_fill_rect(osecpu->window, 0, 0, 0, width, height);
}

void api0010_openWin_default(struct Osecpu* osecpu)
{
	const int orig_r33 = osecpu->registers[0x33];
	const int orig_r34 = osecpu->registers[0x34];
	osecpu->registers[0x33] = 640;
	osecpu->registers[0x34] = 480;
	api0010_openWin(osecpu);
	osecpu->registers[0x33] = orig_r33;
	osecpu->registers[0x34] = orig_r33;
}

void api0002_drawPoint(struct Osecpu* osecpu)
{
	const int mode = osecpu->registers[0x31];
	const int x = osecpu->registers[0x33];
	const int y = osecpu->registers[0x34];
	const int color = osecpu->registers[0x32];
	window_draw_point(osecpu->window, color, x, y);
}

void api0003_drawLine(struct Osecpu* osecpu)
{
	const int mode = osecpu->registers[0x31];
	const int color = osecpu->registers[0x32];
	const int from_x = osecpu->registers[0x33];
	const int from_y = osecpu->registers[0x34];
	const int to_x = osecpu->registers[0x35];
	const int to_y = osecpu->registers[0x36];
	if (!osecpu->window) {
		api0010_openWin_default(osecpu);
	}
	if (mode == MODE_XOR) {
		window_draw_line_xor(osecpu->window, get_color(osecpu, mode&~MODE_XOR, color), from_x, from_y, to_x, to_y);
	} else if (mode == MODE_OR) {
		window_draw_line_or(osecpu->window, get_color(osecpu, mode&~MODE_OR, color), from_x, from_y, to_x, to_y);
	}
}

void api0004_fillRect(struct Osecpu* osecpu)
{
	const int mode = osecpu->registers[0x31];
	const int color = osecpu->registers[0x32];
	const int width = osecpu->registers[0x33];
	const int height = osecpu->registers[0x34];
	const int x = osecpu->registers[0x35];
	const int y = osecpu->registers[0x36];
	const int drawcolor = get_color(osecpu, mode, color);
	if (!osecpu->window) {
		api0010_openWin_default(osecpu);
	}
	if (drawcolor != -1) {
		window_fill_rect(osecpu->window, drawcolor, x, y, width, height);
	}
}

void api0005_fillOval(struct Osecpu* osecpu)
{
	const int mode = osecpu->registers[0x31];
	const int color = osecpu->registers[0x32];
	const int width = osecpu->registers[0x33];
	const int height = (osecpu->registers[0x34] == 0) ? width : osecpu->registers[0x34];
	const int x = osecpu->registers[0x35];
	const int y = osecpu->registers[0x36];
	const int drawcolor = get_color(osecpu, mode, color);
	if (!osecpu->window) {
		api0010_openWin_default(osecpu);
	}
	if (drawcolor != -1) {
		window_fill_oval(osecpu->window, drawcolor, x+width/2, y+height/2, width, height);
	}
}

void api0009_sleep(struct Osecpu* osecpu)
{
	const int opt = osecpu->registers[0x31];
	const int msec = osecpu->registers[0x32];
	usleep(msec*1000);
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
		case API_DRAWLINE:
			api0003_drawLine(osecpu);
			break;
		case API_FILLRECT:
			api0004_fillRect(osecpu);
			break;
		case API_FILLOVAL:
			api0005_fillOval(osecpu);
			break;
		case API_SLEEP:
			api0009_sleep(osecpu);
			break;
		default:
			osecpu->error = ERROR_NOT_IMPLEMENTED_API;
			break;
	}
	osecpu->pregisters[0x3f] = osecpu->pregisters[0x30];
}

