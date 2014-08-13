#include "window.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

struct WindowQueue
{
	enum
	{
		QUEUE_DESTROY,
		QUEUE_REDRAW,
	} type;
	union
	{
	} arg;
};

static int initialized = 0;

void window_destroy(GtkWidget* _window, gpointer data)
{
	struct OsecpuWindow* window = data;
	gtk_widget_destroy(GTK_WIDGET(window->window));
	g_async_queue_unref(window->queue);
	gtk_timeout_remove(window->queue_timer);
	gtk_main_quit();
}

gint queue_timer_event(gpointer data)
{
	struct OsecpuWindow* window = data;
	struct WindowQueue* qdata = g_async_queue_try_pop(window->queue);
	if (qdata) {
		switch (qdata->type) {
			case QUEUE_DESTROY:
				g_signal_emit_by_name(window->window, "destroy");
				break;
			case QUEUE_REDRAW:
				gdk_window_invalidate_rect(gtk_widget_get_window(GTK_WIDGET(window->window)), NULL, 1);
				break;
		}
		free(qdata);
	}
	return TRUE;
}

gboolean expose_event_callback(GtkWidget* widget, GdkEventExpose* event, gpointer data)
{
	struct OsecpuWindow* window = data;
	cairo_t* cr;

	pthread_mutex_lock(&window->surface_mutex);

	cr = gdk_cairo_create(widget->window);
	cairo_set_source_surface(cr, window->surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);

	pthread_mutex_unlock(&window->surface_mutex);
	return TRUE;
}

void* create_window_thread(void* data)
{
	struct OsecpuWindow* window = data;
	window->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title(window->window, "osecpu");
	gtk_window_set_default_size(GTK_WINDOW(window->window), window->initial_width, window->initial_height);
	gtk_window_set_resizable(GTK_WINDOW(window->window), FALSE);
	gtk_window_set_resizable(window->window, FALSE);
	g_signal_connect(window->window, "destroy", G_CALLBACK(window_destroy), window);

	window->drawing = gtk_drawing_area_new();
	gtk_widget_set_size_request(window->drawing, window->initial_width, window->initial_height);
	g_signal_connect(G_OBJECT(window->drawing), "expose_event", G_CALLBACK(expose_event_callback), window);
	gtk_container_add(GTK_CONTAINER(window->window), window->drawing);

	gtk_widget_show_all(GTK_WIDGET(window->window));

	window->queue = g_async_queue_new();
	window->queue_timer = gtk_timeout_add(100, (GtkFunction)queue_timer_event, (gpointer)window);
	gtk_main();
	return NULL;
}

struct OsecpuWindow* window_create(int width, int height)
{
	struct OsecpuWindow* window;
	cairo_t* cr;

	if (!initialized) {
		initialized = 1;
		gtk_init(NULL, NULL);
	}
	window = (struct OsecpuWindow*)malloc(sizeof(struct OsecpuWindow));
	if (!window) return NULL;
	memset(window, 0, sizeof(struct OsecpuWindow));
	window->initial_width = width;
	window->initial_height = height;
	pthread_create(&window->thread, NULL, create_window_thread, window);

	// Initialize a surface
	window->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create(window->surface);
	cairo_set_source_rgb(cr, 255, 255, 255);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	cairo_destroy(cr);

	// Initialize a mutex for a surface
	pthread_mutex_init(&window->surface_mutex, NULL);

	// Wait for creating a winodw and queue
	while (!window->window);
	while (!window->queue);

	return window;
}

void window_free(struct OsecpuWindow* window)
{
	struct WindowQueue* qdata;
	void* dummy;
	qdata = (struct WindowQueue*)malloc(sizeof(struct WindowQueue));
	if (!qdata) return;
	qdata->type = QUEUE_DESTROY;
	g_async_queue_push(window->queue, qdata);
	pthread_join(window->thread, &dummy);
	cairo_surface_destroy(window->surface);
	pthread_mutex_destroy(&window->surface_mutex);
	free(window);
}

void window_fill_rect(struct OsecpuWindow* window, int color, int x, int y, int width, int height)
{
	struct WindowQueue* qdata;
	double r = ((double)((color & 0xff0000) >> 16))/255;
	double g = ((double)((color & 0x00ff00) >>  8))/255;
	double b = ((double)((color & 0x0000ff) >>  0))/255;
	cairo_t* cr;

	pthread_mutex_lock(&window->surface_mutex);

	cr = cairo_create(window->surface);
	cairo_set_source_rgb(cr, r, g, b);
	cairo_rectangle(cr, x, y, width, height);
	cairo_fill(cr);
	cairo_destroy(cr);

	pthread_mutex_unlock(&window->surface_mutex);

	// redraw a window
	qdata = (struct WindowQueue*)malloc(sizeof(struct WindowQueue));
	if (!qdata) return;
	qdata->type = QUEUE_REDRAW;
	g_async_queue_push(window->queue, qdata);
}

void window_draw_point(struct OsecpuWindow* window, int color, int x, int y)
{
	window_fill_rect(window, color, x, y, 1, 1);
}

void window_fill_oval(struct OsecpuWindow* window, int color, int x, int y, int width, int height)
{
	struct WindowQueue* qdata;
	double r = ((double)((color & 0xff0000) >> 16))/255;
	double g = ((double)((color & 0x00ff00) >>  8))/255;
	double b = ((double)((color & 0x0000ff) >>  0))/255;
	cairo_t* cr;

	pthread_mutex_lock(&window->surface_mutex);

	cr = cairo_create(window->surface);
	cairo_set_source_rgb(cr, r, g, b);
	cairo_save(cr);
	cairo_translate(cr, x, y);
	cairo_scale(cr, width/2, height/2);
	cairo_arc(cr, 0, 0, 1.0, 0, M_PI*2);
	cairo_restore(cr);
	cairo_fill(cr);
	cairo_destroy(cr);

	pthread_mutex_unlock(&window->surface_mutex);

	// redraw a window
	qdata = (struct WindowQueue*)malloc(sizeof(struct WindowQueue));
	if (!qdata) return;
	qdata->type = QUEUE_REDRAW;
	g_async_queue_push(window->queue, qdata);
}

