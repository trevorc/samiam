#!/usr/bin/env python

import pygtk
import gtk, gtk.glade, gobject
import sam
import time
import sys

# class CodeTreeModel {{{1
class CodeTreeModel(gtk.GenericTreeModel):
    # TODO Last two may become images
    column_types = (str, int, str, str, str)
    column_names = ['Current', 'Line', 'Breakpoint', 'Code', 'Labels']

    def __init__(self, prog, modnum, bp):
	gtk.GenericTreeModel.__init__(self)
	self._prog = prog
	self._module_n = modnum
	self._instructions = prog.modules[modnum].instructions
	self._breakpoints = bp[modnum]
    
    def on_get_flags(self):
	return gtk.TREE_MODEL_LIST_ONLY | gtk.TREE_MODEL_ITERS_PERSIST
    
    def on_get_n_columns(self):
	return len(self.column_types)

    def on_get_column_type(self, n):
	return self.column_types[n]

    def on_get_iter(self, path):
	return path[0]

    def on_get_path(self, rowref):
	return rowref

    def on_get_value(self, rowref, column):
	item = self._instructions[rowref]
	if column is 0:
	    if self._prog.mc == self._module_n and self._prog.lc == rowref:
		return gtk.STOCK_GO_FORWARD
	    else:
		return None
	elif column is 1:
	    return rowref
	elif column is 2:
	    # TODO breakpoints
	    if rowref in self._breakpoints['normal']:
		return gtk.STOCK_NO #TODO get some pixbufs
	    elif rowref in self._breakpoints['temporary']:
		return gtk.STOCK_YES #TODO get some pixbufs
	    elif rowref == 0:
		return gtk.STOCK_NEW
	    else:
		return None
	elif column is 3:
	    return item.assembly
	elif column is 4:
	    if len(item.labels) > 0:
		return str(item.labels)
	    else:
		return ""
	# TODO and if the column is invalid... ?
	else:
	    return "%(a)s (Error: %(c)d)" % {'a': item.assembly, 'c': column}
    
    def on_iter_next(self, rowref):
	if rowref + 1 < len(self._instructions):
	    return rowref + 1
	else:
	    return None

    def on_iter_children(self, rowref):
	if rowref:
	    return None
	return 0

    def on_iter_has_child(self, rowref):
	return False

    def on_iter_n_children(self, rowref):
	if rowref:
	    return 0
	return len(self._instructions)

    def on_iter_nth_child(self, rowref, n):
	if rowref:
	    return None
	elif n < len(self._instructions):
	    return n
	else:
	    return None

    def on_iter_parent(child):
	return None

# class GSam {{{1
class GSam:
    # __init__ () {{{2
    def __init__(self, filename=None):
	self._xml = gtk.glade.XML('gsam.glade')
	self._xml.signal_autoconnect(self)

	# Getting gtk object handles {{{4
	self._main_window = self._xml.get_widget('main_window')
	self._default_title = self._main_window.get_title()
	self._module_combobox = self._xml.get_widget('module_combobox')
	
	self._code_view = self._xml.get_widget('code_view')
	self._stack_view = self._xml.get_widget('stack_view')
	self._heap_view = self._xml.get_widget('heap_view')
	
	self._code_box = self._xml.get_widget('code_box')
	self._stack_box = self._xml.get_widget('stack_box')
	self._heap_box = self._xml.get_widget('heap_box')

	self._show_code = self._xml.get_widget('show_code')
	self._show_stack = self._xml.get_widget('show_stack')
	self._show_heap = self._xml.get_widget('eshow_heap')

	self._mc_label = self._xml.get_widget('mc_label')
	self._lc_label = self._xml.get_widget('lc_label')
	self._fbr_label = self._xml.get_widget('fbr_label')
	self._sp_label = self._xml.get_widget('sp_label')

	self._console = self._xml.get_widget('console_view')

	self._input_box = self._xml.get_widget('program_entry_box')
	self._input = self._xml.get_widget('program_entry')

	self._custom_dialog = self._xml.get_widget('custom_dialog')
	self._custom_entry = self._xml.get_widget('custom_entry')
	self._custom_speed = self._xml.get_widget('custom_speed')

	# Setup custom speed dialog {{{4
	self._custom_dialog.add_buttons(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,\
                      gtk.STOCK_OK, gtk.RESPONSE_ACCEPT)
	
	# Set default values {{{4
	self._file = None
	self._prog = None
	self._breakpoints = None

	self._timer_id = None
	# TODO decide on best default speeds?
	self._speeds = {'full': 0, 'fast': 10, 'medium': 100, 'slow': 1000}
	self._timeout_interval = self._speeds['fast']

	self._console_end_mark = self._console.get_buffer().create_mark("end",\
		self._console.get_buffer().get_end_iter(), False)
	
	self._module_combobox.set_model(gtk.ListStore(str))
	cell = gtk.CellRendererText()
	self._module_combobox.pack_start(cell, True)
	self._module_combobox.add_attribute(cell, 'text', 0)

	# Code display setup {{{4
	column_names = ['Current', 'Line', 'Breakpoint', 'Code', 'Labels']

	for n in range(0, len(column_names)):
	    if n == 2 or n == 0:
		cell = gtk.CellRendererPixbuf()
		tvcolumn = gtk.TreeViewColumn(column_names[n], cell, text=n)
		tvcolumn.set_attributes(cell, stock_id=n)
	    else:
		cell = gtk.CellRendererText()
		tvcolumn = gtk.TreeViewColumn(column_names[n], cell, text=n)
	    self._code_view.append_column(tvcolumn)
	self._code_view.set_search_column(3)

	# Stack/heap display setup {{{4
	self._stack_view.set_model(gtk.ListStore(gobject.TYPE_LONG,\
	    gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_LONG))

	self._type_colors = {'none': ('lightgray'),\
			    'int':   ('white'),\
			    'float': ('yellow'),\
			    'sa':    ('pink'),\
			    'ha':    ('purple'),\
			    'pa':    ('green'),\
			    'alloc': ('black')}

	def mem_color_code(column, cell, model, iter):
	    type_num = model.get_value(iter, 3)
	    if type_num in range(0, len(sam.Types)):
		row_color = self._type_colors[sam.Types[type_num]]
	    else:
		row_color = self._type_colors['alloc']
		cell.set_property('foreground', 'white')
	    cell.set_property('cell-background', row_color)

	column_names = ['Address', 'Type', 'Value']
	for n in range(0, len(column_names)):
	    renderer = gtk.CellRendererText()
	    column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	    column.set_cell_data_func(renderer, mem_color_code);
	    self._stack_view.append_column(column)
	self._stack_view.set_search_column(1)

	# TODO heap done?
	self._heap_view.set_model(gtk.TreeStore(gobject.TYPE_LONG,\
	    gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_LONG))
	for n in range(0, len(column_names)):
	    renderer = gtk.CellRendererText()
	    column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	    column.set_cell_data_func(renderer, mem_color_code);
	    self._heap_view.append_column(column)
	self._heap_view.set_search_column(1)

	# Handle file input if given {{{4
	if filename:
	    self.open_file(filename)

    ### Open/close file methods {{{2
    # _filechooser_factory () {{{3
    def _filechooser_factory(self, title, p):
	dialog = gtk.FileChooserDialog(title=title, \
		parent=self._main_window,\
		buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,\
		    gtk.STOCK_OPEN,gtk.RESPONSE_OK))
	dialog.set_default_response(gtk.RESPONSE_OK)

	filter = gtk.FileFilter()
	filter.set_name("SaM files")
	filter.add_pattern("*.sam")
	dialog.add_filter(filter)

	filter = gtk.FileFilter()
	filter.set_name("All files")
	filter.add_pattern("*")
	dialog.add_filter(filter)

	return dialog

    # close_file () {{{3
    def close_file(self):
	if self._file:
	    self._file = None
	    self._prog = None
	self._timer_id = None
	self._main_window.set_title(self._default_title)
	self._code_view.set_model(None)
	self._code_models = None
	self._breakpoints = None
	self._stack_view.get_model().clear()
	self._heap_view.get_model().clear()
	self._module_combobox.get_model().clear()

    # on_close_activate () {{{3
    def on_close_activate(self, p):
	self.close_file()

    # open_file () {{{3
    def open_file(self, filename):
	# close old file
	self.close_file()
	self.init_program_display(filename)

    # on_open_activate () {{{3
    def on_open_activate(self, p):
	filechooser = self._filechooser_factory("Select a SaM file", p)
	response = filechooser.run()
	if response == gtk.RESPONSE_OK:
	    self.open_file(filechooser.get_filename())
	filechooser.destroy()

    # init_program_display () {{{3
    def init_program_display(self, filename):
	self._file = filename
	self._main_window.set_title("%(title)s - %(file)s" %
		{'title': self._default_title, 'file': filename})
	self._prog = sam.Program(filename)
	self._prog.print_func = self.sam_print
	self._prog.input_func = self.sam_string_input
	self._finished = False

	mods = self._prog.modules
	modsm = self._module_combobox.get_model()
	for i in range(0, len(mods)):
	    self._module_combobox.append_text("%d - %s" % (i,mods[i].filename))
	self._module_combobox.set_active(0)
	self._breakpoints = [{'normal': set(), 'temporary': set()}\
		for i in range(0, self.get_n_modules())]
	self._code_models = [None for i in range(0, self.get_n_modules())]
	self.update_code_display()
	self.reset_memory_display()

    ### Code display handling {{{2
    def get_selected_code_line(self):
	iter = self._code_view.get_selection().get_selected()[1]
	if iter:
	    return self._code_view.get_model().get_path(iter)[0]
	else:
	    return None

    def refresh_code_display(self):
	self._code_view.queue_draw() # TODO better way to do this?

    def get_n_modules(self):
	return len(self._prog.modules)

    # Note: current module = one being displayed to the user,
    #	not one being executed
    def get_current_module_num(self):
	return self._module_combobox.get_active()

    def get_current_module(self):
	return self._prog.modules[self.get_current_module_num()]

    def get_current_code_model(self):
	return self._code_models[self.get_current_module_num()]

    def set_current_code_model(self, model):
	self._code_models[self.get_current_module_num()] = model

    def update_code_display(self):
	if self.get_current_code_model() is None:
	    self.set_current_code_model(CodeTreeModel(self._prog, \
		    self.get_current_module_num(), self._breakpoints))
	self._code_view.set_model(self.get_current_code_model())

    def on_module_combobox_changed(self, p):
	if self._prog and self._code_models:
	    self.update_code_display()

    def scroll_code_view(self):
	if self._prog.mc == self.get_current_module_num():
	    model = self._code_view.get_model()
	    nIter = model.iter_nth_child(None, self._prog.lc)
	    nPath = model.get_path(nIter)
	    self._code_view.scroll_to_cell(nPath)
	    self.refresh_code_display()

    ### Stack/Heap display handling {{{2
    def value_to_row(self, addr, v):
	if sam.Types[v.type] == 'none':
	    vstr = "-"
	elif sam.Types[v.type] == 'pa':
	    vstr = "%i:%i" % v.value
	else:
	    vstr = "%i" % v.value
	return (addr, sam.TypeChars[v.type], vstr, v.type)

    def none_value_row(self, addr):
	return (addr, 'N', '0', 0)

    def set_row_to_value(self, model, iter, addr, v):
	row = self.value_to_row(addr, v)
	for i in range(0, len(row)):
	    model.set(iter, i, row[i])

    def get_nth_iter(self, model, n):
	return model.iter_nth_child(None, n)

    def get_nth_path(self, model, n):
	return model.get_path(self.get_nth_iter(model, n))

    # Get the iter at the allocation or immediately before where it would go
    def get_heap_allocation_near_iter(self, addr):
	model = self._heap_view.get_model()
	piter = None
	iter = model.get_iter_root()
	# empty tree
	if iter is None:
	    return None
	while iter and model.get_value(iter, 0) <= addr:
	    piter = iter
	    iter = model.iter_next(iter)
	if piter:
	    return piter
	else:
	    return None

    # Get the iter at the allocation exactly
    def get_heap_allocation_iter(self, addr):
	iter = self.get_heap_allocation_near_iter(addr)
	if iter is None:
	    return None
	elif self._heap_view.get_model().get_value(iter, 0) == addr:
	    return iter
	else:
	    return None

    def handle_changes(self):
	for change in self._prog.changes:
	    ctype = sam.ChangeTypes[change.type]
	    if ctype[0:5] == "stack":
		model = self._stack_view.get_model()
	    else:
		model = self._heap_view.get_model()
	    if ctype == "stack_push":
		model.append(self.value_to_row(change.start, change.value))
		self._stack_view.scroll_to_cell(\
			self.get_nth_path(model, change.start))
	    elif ctype == "stack_pop":
		end_iter = model.iter_nth_child(None, model.iter_n_children(\
			None)-1)
		model.remove(end_iter)
	    elif ctype == "stack_change":
		iter = model.iter_nth_child(None, change.start)
		self.set_row_to_value(model, iter, change.start, change.value)
	    elif ctype == "heap_alloc":
		iter = self.get_heap_allocation_near_iter(change.start)
		# TODO heap allocation rows?
		row = (change.start, "", change.size, -1)
		if iter is None:
		    ai = model.insert_before(None, None, row)
		else:
		    ai = model.insert_after(None, iter, row)
		for i in [x + change.start for x in range(0, change.size)]:
		    model.append(ai, self.none_value_row(i))
	    elif ctype == "heap_free":
		iter = self.get_heap_allocation_iter(change.start)
		# iter None here is an error
		model.remove(iter)
	    elif ctype == "heap_change":
		iter = self.get_heap_allocation_near_iter(change.start)
		base = model.get_value(iter, 0)
		n = change.start - base
		riter = model.iter_nth_child(iter, n)
		self.set_row_to_value(model, riter, change.start, change.value)

    ### General display updating {{{2
    def reset_memory_display(self):
	self._stack_view.get_model().clear()
	self._heap_view.get_model().clear()

    def update_registers(self):
	self._mc_label.set_text("MC: %d" % self._prog.mc)
	self._lc_label.set_text("LC: %d" % self._prog.lc) # TODO pc --> lc
	self._fbr_label.set_text("FBR: %d" % self._prog.fbr)
	self._sp_label.set_text("SP: %d" % self._prog.sp)

    def update_display(self):
	self.update_registers()
	self.scroll_code_view()
	self.handle_changes()

    ### Sam program execution {{{2
    # Non-GUI specific implementations {{{3
    # Regular step, ignores breakpoints because it only gets called directly
    # if the user requests a single step
    def step(self):
	if self._prog:
	    if not self._finished:
		self._finished = not self._prog.step()
		if self._finished:
		    self.append_to_console(\
			    "Final Result: %d" % self._prog.stack[0].value)
		    self._timer_id = None
		self.update_display()
	    return not self._finished

    # Single step, ignores breakpoints
    def single_step(self):
	if not self.is_waiting_for_input():
	    return self.step()

    # Step for while running. Supports breakpoints, etc.
    def run_step(self):
	# TODO debug support
	# TODO break before or after breakpoint?
	rv = self.step()
	if self._prog.lc in self._breakpoints[self._prog.mc]['normal']:
	    self._timer_id = None
	    return False
	else:
	    return rv

    def pause(self):
	if not self.is_waiting_for_input():
	    if self._timer_id:
		gobject.source_remove(self._timer_id)
		self._timer_id = None

    def start(self):
	if not self.is_waiting_for_input():
	    if self._prog and self._timer_id is None:
		self._timer_id = gobject.timeout_add(self._timeout_interval,\
			self.run_step)

    def start_pause(self):
	if self._prog:
	    if self._timer_id:
		self.pause()
	    else:
		self.start()

    def reset(self):
	if self._prog:
	    self.pause()
	    self._prog.reset()
	    self.update_registers()
	    self.scroll_code_view()
	    self.reset_memory_display()
	    self._finished = False
	    self._timer_id = None
	    self.reset_input_box()

    # GTK handlers {{{3
    def on_step_clicked(self, p):
	if not self._timer_id:
	    self.single_step()

    def on_start_activate(self, p):
	self.start()

    def on_pause_activate(self, p):
	self.pause()

    def on_start_pause_clicked(self, p):
	self.start_pause()
    
    def on_reset_clicked(self, p):
	self.reset()

    ### Breakpoints {{{2
    def toggle_breakpoint(self, module, line):
	bp = self._breakpoints[module]['normal']
	if line in bp:
	    bp.remove(line)
	else:
	    bp.add(line)
	self.refresh_code_display()

    def toggle_breakpoint_selected(self):
	if self._prog:
	    self.toggle_breakpoint(self.get_current_module_num(),\
		    self.get_selected_code_line())

    # TODO semantics of non-normal breakpoints?
    def clear_breakpoints(self):
	if self._breakpoints:
	    for bps in self._breakpoints:
		for key in bps:
		    bps[key].clear()
	self.refresh_code_display()

    def on_code_view_row_activated(self, tv, path, col):
	if self._prog:
	    self.toggle_breakpoint(self.get_current_module_num(), path[0])

    def on_toggle_breakpoint_activate(self, p):
	self.toggle_breakpoint_selected()

    def on_remove_all_breakpoints_activate(self, p):
	self.clear_breakpoints()

    ### View menu handling {{{2
    def on_show_code_activate(self, p):
	if self._show_code.get_active():
	    self._code_box.show_all()
	else:
	    self._code_box.hide_all()

    def on_show_stack_activate(self, p):
	if self._show_stack.get_active():
	    self._stack_box.show_all()
	else:
	    self._stack_box.hide_all()

    def on_show_heap_activate(self, p):
	if self._show_heap.get_active():
	    self._heap_box.show_all()
	else:
	    self._heap_box.hide_all()

    ### Console handling {{{2
    # append_to_console () {{{3
    def append_to_console(self, str):
	buf = self._console.get_buffer()
	if buf.get_char_count() == 0:
	    st = ""
	else:
	    st = "\n"
	buf.insert(buf.get_end_iter(),\
		"%s[%s] %s" % (st, time.strftime('%X'), str))
	self._console.scroll_mark_onscreen(self._console_end_mark)

    # on_clear_console {{{3
    def on_clear_console_activate(self, p):
	self._console.get_buffer().set_text("")

    ### Speed change handlers {{{2
    # Requires: newSpeed >= 0
    def change_speed(self, newSpeed):
	self._timeout_interval = newSpeed
	if self._prog and self._timer_id:
	    self.pause()
	    self.start()

    def change_to_def_speed(self, newSpeedIndex):
	self.change_speed(self._speeds[newSpeedIndex])

    def on_full_speed_activate(self, p):
	self.change_to_def_speed('full')

    def on_fast_speed_activate(self, p):
	self.change_to_def_speed('fast')

    def on_medium_speed_activate(self, p):
	self.change_to_def_speed('medium')

    def on_slow_speed_activate(self, p):
	self.change_to_def_speed('slow')

    def on_custom_speed_activate(self, p):
	if self._custom_speed.get_active():
	    self._custom_entry.set_value(self._timeout_interval)
	    if self._custom_dialog.run() == gtk.RESPONSE_ACCEPT:
		customSpeed = self._custom_entry.get_value_as_int()
		self.change_speed(customSpeed)
	    # Otherwise leave speed as it is, but custom is selected
	    self._custom_dialog.hide()
    
    def on_custom_dialog_ok(self, p):
	self._custom_dialog.response(gtk.RESPONSE_ACCEPT)

    ### IO Funcs {{{2
    def is_waiting_for_input(self):
	self._input_box.get_property("visible")

    def reset_input_box(self):
	if self._input_box.get_property("visible"):
	    self._input_ready = False
	    self._input_box.hide_all()
	    gtk.main_quit()

    def sam_string_input(self):
	self._input.set_text("")
	self._input_ready = False
	self._input_box.show_all()
	self._input.grab_focus() # Hmm... focus grabbing...
	# TODO Hang until response, fonctionne?
	gtk.main()
	if self._input_ready:
	    res = self._input.get_text()
	    self._input.set_text("")
	    self._input_box.hide_all()
	    self.append_to_console("User Input: %s" % res)
	    return res
	else: # TODO failure?
	    return self.sam_string_input()

    def on_program_entry_activate(self, p):
	self._input_ready = True
	gtk.main_quit()
	# TODO return res in on_input_requested

    def sam_print(self, str):
	self.append_to_console("Program output: %s" % str)

    # gtk_main_quit () {{{2
    def gtk_main_quit(*self):
	gtk.main_quit()

# main {{{1
if __name__ == '__main__':
    if len(sys.argv) > 1:
	gsam = GSam(sys.argv[1])
    else:
	gsam = GSam()
	gsam.on_open_activate(None)

    gtk.main()
