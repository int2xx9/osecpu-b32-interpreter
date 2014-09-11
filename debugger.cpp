#include "debugger.h"
extern "C" {
#include "reverse_aska.h"
#include "window.h"
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtkmm.h>

class ControlWidget : public Gtk::HBox
{
	Gtk::Button button_continue_pause;
	Gtk::Button button_step;
	const OsecpuDebugger& debugger;
public:
	ControlWidget(const OsecpuDebugger& debugger) :
		button_continue_pause("Continue / Pause"),
		button_step("Step"),
		debugger(debugger)
	{
		pack_start(button_continue_pause, Gtk::PACK_SHRINK);
		pack_start(button_step, Gtk::PACK_SHRINK);

		button_continue_pause.signal_clicked().connect(sigc::mem_fun(this, &ControlWidget::on_continue_pause_button_clicked));
		button_step.signal_clicked().connect(sigc::mem_fun(this, &ControlWidget::on_step_button_clicked));
	}

	void on_continue_pause_button_clicked();
	void on_step_button_clicked();
};

class RegistersWidget : public Gtk::VBox
{
	Gtk::Label label;
	Gtk::ScrolledWindow scrwin;
	Gtk::TreeView treeview;
	Gtk::TreeModelColumn<Glib::ustring> name;
	Gtk::TreeModelColumn<int> value;
	Gtk::TreeModelColumn<Glib::ustring> type;
	Gtk::TreeModel::ColumnRecord record;
	Glib::RefPtr<Gtk::ListStore> liststore;
	const OsecpuDebugger& debugger;
public:
	RegistersWidget(const OsecpuDebugger& debugger) :
		label("Registers", Gtk::ALIGN_START, Gtk::ALIGN_LEFT),
		debugger(debugger)
	{
		pack_start(label, Gtk::PACK_SHRINK);
		scrwin.add(treeview);
		scrwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		pack_start(scrwin);

		record.add(name);
		record.add(value);
		record.add(type);
		liststore = Gtk::ListStore::create(record);
		treeview.set_model(liststore);
		treeview.append_column("Name", name);
		treeview.append_column("Value", value);
		treeview.append_column("Type", type);
	}

	int Reload()
	{
		Gtk::TreeModel::Row row;
		int i;

		liststore->clear();

		for (i = 0; i < 0x40; i++) {
			char regname_c[4];
			Glib::ustring regname;
			Glib::ustring type_name("-");

			snprintf(regname_c, 4, "R%02X", i);
			regname = regname_c;

			row = *liststore->append();
			row[name] = regname;
			row[value] = debugger.osecpu->registers[i];
			row[type] = type_name;
		}

		for (i = 0; i < 0x40; i++) {
			char regname_c[4];
			char type_name_c[16];
			Glib::ustring regname;
			Glib::ustring type_name;

			snprintf(regname_c, 4, "P%02X", i);
			regname = regname_c;

			snprintf(type_name_c, 16, "%d", debugger.osecpu->pregisters[i].type);
			type_name = type_name_c;

			row = *liststore->append();
			row[name] = regname;
			row[value] = debugger.osecpu->pregisters[i].p.code;
			row[type] = type_name;
		}

		for (i = 0; i < 4; i++) {
			char regname_c[4];
			char type_name_c[16];
			Glib::ustring regname;
			Glib::ustring type_name("-");

			snprintf(regname_c, 4, "D%02X", i);
			regname = regname_c;

			row = *liststore->append();
			row[name] = regname;
			row[value] = debugger.osecpu->dregisters[i];
			row[type] = type_name;
		}

		return 1;
	}
};

class LabelsWidget : public Gtk::VBox
{
	Gtk::Label label;
	Gtk::ScrolledWindow scrwin;
	Gtk::TreeView treeview;
	Gtk::TreeModelColumn<int> number;
	Gtk::TreeModelColumn<int> codepos;
	Gtk::TreeModelColumn<Glib::ustring> type;
	Gtk::TreeModel::ColumnRecord record;
	Glib::RefPtr<Gtk::ListStore> liststore;
	const OsecpuDebugger& debugger;
public:
	LabelsWidget(const OsecpuDebugger& debugger) :
		label("Labels", Gtk::ALIGN_START, Gtk::ALIGN_LEFT),
		debugger(debugger)
	{
		pack_start(label, Gtk::PACK_SHRINK);
		scrwin.add(treeview);
		scrwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		pack_start(scrwin);

		record.add(number);
		record.add(codepos);
		record.add(type);
		liststore = Gtk::ListStore::create(record);
		treeview.set_model(liststore);
		treeview.append_column("#", number);
		treeview.append_column("Pos", codepos);
		treeview.append_column("Type / Length", type);
	}

	int Reload()
	{
		Gtk::TreeModel::Row row;
		int i;

		liststore->clear();

		for (i = 0; i < debugger.osecpu->labelcnt; i++) {
			Glib::ustring type_name;

			if (debugger.osecpu->labels[i].data) {
				char type_name_c[32];
				snprintf(type_name_c, 32, "data (T_UINT8) (%dbytes)", debugger.osecpu->labels[i].datalen);
				type_name = type_name_c;
			} else {
				type_name = "code";
			}

			row = *liststore->append();
			row[number] = debugger.osecpu->labels[i].id;
			row[codepos] = debugger.osecpu->labels[i].pos;
			row[type] = type_name;
		}

		return 1;
	}
};

class CodeWidget : public Gtk::VBox
{
	Gtk::Label label;
	Gtk::ScrolledWindow scrwin;
	Gtk::TreeView treeview;
	Gtk::TreeModelColumn<int> inst_number;
	Gtk::TreeModelColumn<Glib::ustring> inst_string;
	Gtk::TreeModel::ColumnRecord record;
	Glib::RefPtr<Gtk::ListStore> liststore;
	const OsecpuDebugger& debugger;

	void on_cell_data(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& it)
	{
		Gtk::TreeModel::Path path = liststore->get_path(it);
		Gtk::TreeModel::Row row = *it;
		if (debugger.osecpu->status == OSECPU_STATUS_NOT_INITIAlIZED && row[inst_number] == 0) {
			cell->property_cell_background_gdk() = Gdk::Color("#ffff99");
		} else if (debugger.osecpu->status == OSECPU_STATUS_PAUSED && row[inst_number] == debugger.osecpu->pregisters[0x3f].p.code) {
			cell->property_cell_background_gdk() = Gdk::Color("#ffff99");
		} else {
			cell->property_cell_background_gdk() = Gdk::Color("#ffffff");
		}
	}
public:
	CodeWidget(const OsecpuDebugger& debugger) :
		label("Code", Gtk::ALIGN_START, Gtk::ALIGN_LEFT),
		debugger(debugger)
	{
		Gtk::CellRenderer* render;
		pack_start(label, Gtk::PACK_SHRINK);
		scrwin.add(treeview);
		scrwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		pack_start(scrwin);

		record.add(inst_number);
		record.add(inst_string);
		liststore = Gtk::ListStore::create(record);
		treeview.set_model(liststore);
		treeview.append_column("Instruction #", inst_number);
		treeview.append_column("ASKA", inst_string);

		render = treeview.get_column_cell_renderer(0);
		treeview.get_column(0)->set_cell_data_func(*render, sigc::mem_fun(this, &CodeWidget::on_cell_data));
		render = treeview.get_column_cell_renderer(1);
		treeview.get_column(1)->set_cell_data_func(*render, sigc::mem_fun(this, &CodeWidget::on_cell_data));
	}

	int Reload()
	{
		ReverseAska* raska;
		Gtk::TreeModel::Row row;
		int i;

		liststore->clear();

		raska = reverse_aska_init(debugger.osecpu->orig_code, debugger.osecpu->orig_codelen);
		if (!raska) return 0;

		for (i = 0; i < raska->idxcnt; i++) {
			// FIXME: inst, inst->inst_str are leaked memory
			struct ReverseAskaInstruction* inst;
			inst = get_instruction_string(raska, i);
			row = *liststore->append();
			row[inst_number] = i;
			row[inst_string] = inst->inst_str;
		}

		reverse_aska_free(raska);

		return 1;
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
	const OsecpuDebugger& debugger;
public:
	DebuggerWindow(const OsecpuDebugger& debugger) :
		debugger(debugger),
		widget_control(debugger),
		widget_registers(debugger),
		widget_labels(debugger),
		widget_code(debugger)
	{
		resize(800, 500);
		set_title("OSECPU Debugger");
		vbox_left.pack_start(widget_control, Gtk::PACK_SHRINK);
		vbox_left.pack_start(widget_registers);
		vbox_left.pack_start(widget_labels);
		vbox_left.set_size_request(300);
		vbox_right.pack_start(widget_code);
		hbox.pack_start(vbox_left, Gtk::PACK_SHRINK);
		hbox.pack_start(vbox_right);
		vbox.pack_start(hbox);
		add(vbox);
		show_all_children();

		Reload();
	}

	void Reload() {
		widget_registers.Reload();
		widget_labels.Reload();
		widget_code.Reload();
	}
};

void ControlWidget::on_continue_pause_button_clicked()
{
	struct OsecpuCommand* cmd = (struct OsecpuCommand*)malloc(sizeof(struct OsecpuCommand));
	if (debugger.osecpu->status == OSECPU_STATUS_RUNNING) {
		cmd->type = OsecpuCommand::PAUSE_REQUEST;
		g_async_queue_push(debugger.osecpu->osecpu_thread_queue, cmd);
		while (debugger.osecpu->status != OSECPU_STATUS_PAUSED);
		((DebuggerWindow*)debugger.window)->Reload();
		return;
	}
	if (debugger.osecpu->status == OSECPU_STATUS_NOT_INITIAlIZED) {
		cmd->type = OsecpuCommand::RESTART;
	} else if (debugger.osecpu->status == OSECPU_STATUS_RUNNING) {
		cmd->type = OsecpuCommand::PAUSE_REQUEST;
	} else if (debugger.osecpu->status == OSECPU_STATUS_PAUSED) {
		cmd->type = OsecpuCommand::CONTINUE;
	}
	g_async_queue_push(debugger.osecpu->osecpu_thread_queue, cmd);
}

void ControlWidget::on_step_button_clicked()
{
	if (debugger.osecpu->status == OSECPU_STATUS_NOT_INITIAlIZED) {
		struct OsecpuCommand* cmd = (struct OsecpuCommand*)malloc(sizeof(struct OsecpuCommand));
		cmd->type = OsecpuCommand::INITIALIZE;
		g_async_queue_push(debugger.osecpu->osecpu_thread_queue, cmd);
		cmd = (struct OsecpuCommand*)malloc(sizeof(struct OsecpuCommand));
		cmd->type = OsecpuCommand::NEXT;
		g_async_queue_push(debugger.osecpu->osecpu_thread_queue, cmd);

	} else if (debugger.osecpu->status == OSECPU_STATUS_PAUSED) {
		struct OsecpuCommand* cmd = (struct OsecpuCommand*)malloc(sizeof(struct OsecpuCommand));
		cmd->type = OsecpuCommand::NEXT;
		g_async_queue_push(debugger.osecpu->osecpu_thread_queue, cmd);
	}
	((DebuggerWindow*)debugger.window)->Reload();
}

void* create_debugger_window_thread(void* data)
{
	OsecpuDebugger* debugger = (OsecpuDebugger*)data;
	Gtk::Main gtk(NULL, NULL);
	debugger->window = new DebuggerWindow(*debugger);
	Gtk::Main::run(*(DebuggerWindow*)debugger->window);
	debugger->window = NULL;
}

extern "C" OsecpuDebugger* debugger_init(struct Osecpu* osecpu)
{
	OsecpuDebugger* debugger;
	debugger = (OsecpuDebugger*)malloc(sizeof(OsecpuDebugger));
	if (!debugger) return NULL;
	memset(debugger, 0, sizeof(OsecpuDebugger));
	debugger->osecpu = osecpu;

	return debugger;
}

extern "C" void debugger_free(OsecpuDebugger* debugger)
{
	free(debugger->checkpoint);
	free(debugger);
}

extern "C" void debugger_open(OsecpuDebugger* debugger)
{
	char cmdbuf[1024];

	if (!debugger->window) {
		pthread_create(&debugger->windowThread, NULL, create_debugger_window_thread, debugger);
		while (!debugger->window);
	}

	while (1) {
		printf("debug> ");
		fgets(cmdbuf, 1024, stdin);
		cmdbuf[strlen(cmdbuf)-1] = 0;
		if (strcmp(cmdbuf, "continue") == 0) {
			if (debugger->osecpu->status == OSECPU_STATUS_NOT_INITIAlIZED) {
				restart_osecpu(debugger->osecpu);
				if (wait_osecpu_exit(debugger->osecpu) == 2) {
					printf("breakpoint.\n");
					debugger_open(debugger);
				}
			} else if (debugger->osecpu->status == OSECPU_STATUS_PAUSED) {
				continue_osecpu(debugger->osecpu);
				if (wait_osecpu_exit(debugger->osecpu) == 2) {
					printf("breakpoint.\n");
					debugger_open(debugger);
				}
			}
			return;
		} else if (strcmp(cmdbuf, "next") == 0) {
			do_next_instruction(debugger->osecpu);
			if (debugger->osecpu->pregisters[0x3f].p.code >= debugger->osecpu->codelen) {
				return;
			}
		} else if (strcmp(cmdbuf, "coredump") == 0) {
			coredump(debugger->osecpu);
		} else if (strcmp(cmdbuf, "checkpoint") == 0) {
			int i;
			if (!debugger->checkpoint) {
				debugger->checkpoint = (struct Osecpu*)malloc(sizeof(struct Osecpu));
			}
			memcpy(debugger->checkpoint, debugger->osecpu, sizeof(struct Osecpu));
			debugger->checkpoint_surface = window_copy_surface(NULL, debugger->osecpu->window->surface);
			debugger->checkpoint_labels = (struct Label*)malloc(sizeof(struct Label) * debugger->osecpu->labelcnt);
			for (i = 0; i < debugger->osecpu->labelcnt; i++) {
				debugger->checkpoint_labels[i] = debugger->osecpu->labels[i];
				if (debugger->checkpoint_labels[i].data) {
					debugger->checkpoint_labels[i].data = (uint8_t*)malloc(debugger->checkpoint_labels[i].datalen);
					memcpy(debugger->checkpoint_labels[i].data, debugger->osecpu->labels[i].data, debugger->checkpoint_labels[i].datalen);
				}
			}
			debugger->checkpoint_labelcnt = debugger->osecpu->labelcnt;
		} else if (strcmp(cmdbuf, "replay") == 0) {
			int i;
			struct OsecpuCommand* cmd = (struct OsecpuCommand*)malloc(sizeof(struct OsecpuCommand));
			OsecpuWindow* window;
			struct Label* labels;
			cmd->type = OsecpuCommand::PAUSE_REQUEST;
			g_async_queue_push(debugger->osecpu->osecpu_thread_queue, cmd);
			window = debugger->osecpu->window;
			labels = debugger->osecpu->labels;
			memcpy(debugger->osecpu, debugger->checkpoint, sizeof(struct Osecpu));
			debugger->osecpu->window = window;
			debugger->osecpu->labels = labels;
			window_copy_surface(window->surface, debugger->checkpoint_surface);
			for (i = 0; i < debugger->checkpoint_labelcnt; i++) {
				if (debugger->osecpu->labels[i].data) {
					memcpy(debugger->osecpu->labels[i].data, debugger->checkpoint_labels[i].data, debugger->osecpu->labels[i].datalen);
				}
			}
		} else {
			printf("command: continue, next, coredump, checkpoint, replay\n");
		}
	}
}

