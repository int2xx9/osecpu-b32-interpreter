#include "debugger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtkmm.h>

class ControlWidget : public Gtk::HBox
{
	Gtk::Button button_continue_pause;
	Gtk::Button button_stepover;
public:
	ControlWidget() :
		button_continue_pause("Continue / Pause"),
		button_stepover("Stepover")
	{
		pack_start(button_continue_pause, Gtk::PACK_SHRINK);
		pack_start(button_stepover, Gtk::PACK_SHRINK);
	}
};

class RegistersWidget : public Gtk::VBox
{
	Gtk::Label label;
	Gtk::ScrolledWindow scrwin;
	Gtk::TreeView treeview;
public:
	RegistersWidget() : label("Registers", Gtk::ALIGN_START, Gtk::ALIGN_LEFT)
	{
		pack_start(label, Gtk::PACK_SHRINK);
		scrwin.add(treeview);
		pack_start(scrwin);
	}
};

class LabelsWidget : public Gtk::VBox
{
	Gtk::Label label;
	Gtk::ScrolledWindow scrwin;
	Gtk::TreeView treeview;
public:
	LabelsWidget() : label("Labels", Gtk::ALIGN_START, Gtk::ALIGN_LEFT)
	{
		pack_start(label, Gtk::PACK_SHRINK);
		scrwin.add(treeview);
		pack_start(scrwin);
	}
};

class CodeWidget : public Gtk::VBox
{
	Gtk::Label label;
	Gtk::ScrolledWindow scrwin;
	Gtk::TreeView treeview;
public:
	CodeWidget() : label("Code", Gtk::ALIGN_START, Gtk::ALIGN_LEFT)
	{
		pack_start(label, Gtk::PACK_SHRINK);
		scrwin.add(treeview);
		pack_start(scrwin);
	}
};

class DebuggerWindow : public Gtk::Window
{
	Gtk::VBox vbox, vbox_left, vbox_right;
	Gtk::HBox hbox;
	ControlWidget widget_control;
	RegistersWidget widget_registers;
	LabelsWidget widget_labels;
	CodeWidget widget_code;
public:
	DebuggerWindow()
	{
		set_title("OSECPU Debugger");
		vbox_left.pack_start(widget_control, Gtk::PACK_SHRINK);
		vbox_left.pack_start(widget_registers);
		vbox_left.pack_start(widget_labels);
		vbox_right.pack_start(widget_code);
		hbox.pack_start(vbox_left);
		hbox.pack_start(vbox_right);
		vbox.pack_start(hbox);
		add(vbox);
		show_all_children();
	}
};

void* create_debugger_window_thread(void* data)
{
	OsecpuDebugger* debugger = (OsecpuDebugger*)data;
	Gtk::Main gtk(NULL, NULL);
	debugger->window = new DebuggerWindow();
	Gtk::Main::run(*(DebuggerWindow*)debugger->window);
}

extern "C" OsecpuDebugger* debugger_init(struct Osecpu* osecpu)
{
	OsecpuDebugger* debugger;
	debugger = (OsecpuDebugger*)malloc(sizeof(OsecpuDebugger));
	if (!debugger) return NULL;
	memset(debugger, 0, sizeof(OsecpuDebugger));
	debugger->osecpu = osecpu;

	pthread_create(&debugger->windowThread, NULL, create_debugger_window_thread, debugger);

	while (!debugger->window);
	((Gtk::Window*)debugger->window)->show_all_children();

	return debugger;
}

extern "C" void debugger_free(OsecpuDebugger* debugger)
{
	free(debugger);
}

extern "C" void debugger_open(OsecpuDebugger* debugger)
{
	char cmdbuf[1024];
	while (1) {
		printf("debug> ");
		fgets(cmdbuf, 1024, stdin);
		cmdbuf[strlen(cmdbuf)-1] = 0;
		if (strcmp(cmdbuf, "continue") == 0) {
			if (continue_osecpu(debugger->osecpu) == 2) {
				printf("breakpoint.\n");
				debugger_open(debugger);
			}
			return;
		} else if (strcmp(cmdbuf, "next") == 0) {
			do_next_instruction(debugger->osecpu);
			if (debugger->osecpu->pregisters[0x3f].p.code >= debugger->osecpu->codelen) {
				return;
			}
		} else if (strcmp(cmdbuf, "coredump") == 0) {
			coredump(debugger->osecpu);
		} else {
			printf("command: continue, next, coredump\n");
		}
	}
}

