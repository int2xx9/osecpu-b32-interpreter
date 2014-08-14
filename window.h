#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <gtk/gtk.h>
#include <pthread.h>

struct OsecpuWindow
{
	pthread_t thread;
	GtkWindow* window;
	GtkWidget* drawing;
	GAsyncQueue* queue;
	gint queue_timer;
	int initial_width;
	int initial_height;
	pthread_mutex_t surface_mutex;
	cairo_surface_t* surface;
};

struct OsecpuWindow* window_create();
void window_free(struct OsecpuWindow*);
void window_draw_point(struct OsecpuWindow*, int, int, int);
void window_fill_rect(struct OsecpuWindow*, int, int, int, int, int);
void window_fill_oval(struct OsecpuWindow*, int, int, int, int, int);
void window_draw_line(struct OsecpuWindow*, int, int, int, int, int);
void window_draw_line_or(struct OsecpuWindow*, int, int, int, int, int);
void window_draw_line_xor(struct OsecpuWindow*, int, int, int, int, int);

#endif

