#!/usr/bin/env python

import pygtk
import gtk, gtk.glade, gobject
import sam
import time
import sys

# prepare_value_view {{{1
# make_value_tree_store {{{3
def make_value_tree_store():
    return gtk.TreeStore(gobject.TYPE_LONG,\
	gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_LONG)

def prepare_value_view(view):
    type_colors = {'none': ('lightgray'),\
			'int':   ('white'),\
			'float': ('yellow'),\
			'sa':    ('pink'),\
			'ha':    ('purple'),\
			'pa':    ('green'),\
			'block': ('black')}
    def mem_color_code(column, cell, model, iter):
	type_num = model.get_value(iter, 3)
	if type_num in range(0, len(sam.Types)):
	    row_color = type_colors[sam.Types[type_num]]
	    cell.set_property('foreground', 'black')
	else:
	    row_color = type_colors['block']
	    cell.set_property('foreground', 'white')
	cell.set_property('cell-background', row_color)

    column_names = ['Address', 'Type', 'Value']
    view.set_model(make_value_tree_store())
    for n in range(0, len(column_names)):
	renderer = gtk.CellRendererText()
	column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	column.set_cell_data_func(renderer, mem_color_code);
	view.append_column(column)
    view.set_search_column(1)


# class CodeTreeModel {{{1
class CodeTreeModel(gtk.GenericTreeModel):
    column_types = (str, int, gtk.gdk.Pixbuf, str, str)
    column_names = ['Current', 'Line', 'Breakpoint', 'Code', 'Labels']

    def __init__(self, prog, modnum, bp, bpnorm, bptemp):
	gtk.GenericTreeModel.__init__(self)
	self._prog = prog
	self._module_n = modnum
	self._instructions = prog.modules[modnum].instructions
	self._breakpoints = bp[modnum]

	self._blank_pixbuf = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8,\
		bpnorm.get_width(), bpnorm.get_height())
	self._bp_norm_pixbuf = bpnorm
	self._bp_temp_pixbuf = bptemp
   
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
	    if rowref in self._breakpoints['normal']:
		return self._bp_norm_pixbuf
	    elif rowref in self._breakpoints['temporary']:
		return self._bp_temp_pixbuf
	    # Hack to fix "pixbuf required" bug
	    elif rowref == 0:
		return self._blank_pixbuf
	    else:
		return None
	elif column is 3:
	    return item.assembly
	elif column is 4:
	    if len(item.labels) > 0:
		return str(item.labels)
	    else:
		return ""
	# and if the column is invalid... (error: it never happens)
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

# class Capture {{{1
class Capture:
    # __init__ () {{{2
    def __init__(self, data):
	self._xml = gtk.glade.XML('gsam_capture.glade')
	self._xml.signal_autoconnect(self)

	self._main_window = self._xml.get_widget('main_window')
	self._instructions_view = self._xml.get_widget('instructions_view')

	widget_prefixes = ('inst', 'pc_label', 'fbr_label', 'sp_label',\
		'stack_view', 'heap_view')
	self._widgets = [ map(self._xml.get_widget,\
		map(lambda x: "%s%d" % (x, i), widget_prefixes))\
		for i in range(1, 6) ]
	map(prepare_value_view, map(self._xml.get_widget,\
		["stack_view%d" % i for i in range(1, 6)]))

	self._data = data

	for i in range(0, len(self._widgets)):
	    if i < len(data):
		self.show_data_at(data[i], i)

	# Setup instructions view {{{4
	column_names = ['Address', 'Code']
	self._instructions_view.set_model(gtk.ListStore(str, str))
	for n in range(0, len(column_names)):
	    renderer = gtk.CellRendererText()
	    column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	    self._instructions_view.append_column(column)
	self._instructions_view.set_search_column(1)
	
	# Insert data into instructions view {{{4
	for row in data:
	    self._instructions_view.get_model().append(\
		    ("%i:%i" % (row['mc'], row['lc']), row['code']))

    # show_data {{{2
    def show_data_in(self, row, widgets):
	(code_label, pc_label, fbr_label, sp_label, stack, heap) = widgets
	if code_label:
	    code_label.set_text('Stack after "%s"' % row['code'])
	if pc_label:
	    pc_label.set_text("%i:%i" % (row['mc'], row['lc']))
	if fbr_label:
	    fbr_label.set_text("%i" % row['fbr'])
	if sp_label:
	    sp_label.set_text("%i" % row['sp'])
	if stack:
	    stack.set_model(row['stack'])
	    stack.expand_all()
	if heap:
	    heap.set_model(row['heap'])
	    heap.expand_all()

    def show_data_at(self, row, n):
	self.show_data_in(row, self._widgets[n])

    def on_instructions_view_cursor_changed(self, p):
	(path, col) = p.get_cursor()
	i = path[0]
	num_widgets = len(self._widgets)
	edge_dist = num_widgets / 2
	num_insts = len(self._data)
	if num_widgets > num_insts:
	    return
	min = i
	max = i + num_widgets
	if min - edge_dist >= 0:
	    min = min - edge_dist
	    max = max - edge_dist
	if max > num_insts:
	    diff = max - num_insts
	    min = min - diff
	    max = max - diff
	for i in range(min, max):
	    if i >= 0 and i < len(self._data):
		self.show_data_at(self._data[i], i - min)

    # show and hide {{{2
    def show(self):
	self._main_window.show()

    def hide(self):
	self._main_window.hide()

    def on_close_activate(self, p):
	self.hide()

# class GSam {{{1
class GSam:
    # initization {{{2
    # __init__ () {{{3
    def __init__(self, filename=None):
	self._xml = gtk.glade.XML('gsam.glade')
	self._xml.signal_autoconnect(self)

	# Getting gtk object handles {{{4
	self._main_window = self._xml.get_widget('main_window')
	self._default_title = self._main_window.get_title()

	self._about_dialog = self._xml.get_widget('about_dialog')

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
	self._break_return = None
	self._capture = None

	self._timer_id = None
	self._speeds = {'full': 0, 'fast': 10, 'medium': 100, 'slow': 1000}
	self._timeout_interval = self._speeds['fast']

	self._console_end_mark = self._console.get_buffer().create_mark("end",\
		self._console.get_buffer().get_end_iter(), False)
	
	self._module_combobox.set_model(gtk.ListStore(str))
	cell = gtk.CellRendererText()
	self._module_combobox.pack_start(cell, True)
	self._module_combobox.add_attribute(cell, 'text', 0)

	# Code display setup {{{4
	# TODO better pixbufs?
	self._bp_norm = self._main_window.render_icon(gtk.STOCK_NO,\
		gtk.ICON_SIZE_MENU)
	self._bp_temp = self._main_window.render_icon(gtk.STOCK_YES,\
		gtk.ICON_SIZE_MENU)

	column_names = ['Current', 'Line', 'Breakpoint', 'Code', 'Labels']

	for n in range(0, len(column_names)):
	    if n == 2 or n == 0:
		cell = gtk.CellRendererPixbuf()
		tvcolumn = gtk.TreeViewColumn(column_names[n], cell, text=n)
		if n == 0:
		    tvcolumn.set_attributes(cell, stock_id=n)
		if n == 2:
		    tvcolumn.set_attributes(cell, pixbuf=n)
	    else:
		cell = gtk.CellRendererText()
		tvcolumn = gtk.TreeViewColumn(column_names[n], cell, text=n)
	    self._code_view.append_column(tvcolumn)
	self._code_view.set_search_column(3)

	# Stack/heap display setup {{{4
	prepare_value_view(self._stack_view)
	prepare_value_view(self._heap_view)

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
	    self.append_banner_to_console("CLOSED %s" % self._file)
	    self._file = None
	    self._prog = None
	self._timer_id = None
	self._capture = None
	self._main_window.set_title(self._default_title)
	self._code_view.set_model(None)
	self._code_models = None
	self._breakpoints = None
	self._break_return = None
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
	try:
	    self._file = filename
	    self._prog = sam.Program(filename)
	    self._prog.print_func = self.sam_print
	    self._prog.input_func = self.sam_string_input
	    self._finished = False

	    mods = self._prog.modules
	    modsm = self._module_combobox.get_model()
	    for i in range(0, len(mods)):
		self._module_combobox.append_text("%d - %s" %
			(i,mods[i].filename))
	    self._module_combobox.set_active(0)
	    self._breakpoints = [{'normal': set(), 'temporary': set()}\
		    for i in range(0, self.get_n_modules())]
	    self._code_models = [None for i in range(0, self.get_n_modules())]
	    self.update_code_display()
	    self.reset_memory_display()

	    self._main_window.set_title("%(title)s - %(file)s" %
		    {'title': self._default_title, 'file': filename})
	    self.append_banner_to_console("OPENED %s" % filename)
	except sam.ParseError:
	    self._file = None
	    self._prog = None
	    self.append_to_console("Parse error loading file %s" % filename,\
		    False)

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
	    self.set_current_code_model(CodeTreeModel(self._prog,\
		    self.get_current_module_num(), self._breakpoints,\
		    self._bp_norm, self._bp_temp))
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
    # Helpers {{{3
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

    def get_nth_iter(self, model, iter, n):
	if n < 0:
	    return None
	else:
	    return model.iter_nth_child(iter, n)

    def get_last_iter(self, model, iter):
	return self.get_nth_iter(model, iter, model.iter_n_children(iter)-1)

    # Get the iter at the allocation or immediately before where it would go
    def get_block_near_iter(self, model, addr):
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
    def get_block_iter(self, model, addr):
	iter = self.get_block_near_iter(model, addr)
	if iter is None:
	    return None
	elif model.get_value(iter, 0) == addr:
	    return iter
	else:
	    return None

    # handle_changes () {{{3
    def handle_changes(self):
	for change in self._prog.changes:
	    ctype = sam.ChangeTypes[change.type]
	    if ctype[0:5] == "stack":
		model = self._stack_view.get_model()
	    else:
		model = self._heap_view.get_model()
	    if ctype == "stack_push":
		v = self.value_to_row(change.start, change.value)
		last = self.get_last_iter(model, None)
		if last is None:
		    fbr = None
		else:
		    fbr = model.get_value(last, 0)
		if last is None or fbr is None or fbr < self._prog.fbr:
		    # TODO smarter way to do this?
		    insts = self._prog.modules[self._prog.mc].instructions
		    prev = insts[self._prog.lc - 1]
		    if len(prev.labels) == 0:
			label = None
		    else:
			label = prev.labels[0]

		    last = model.append(None,\
			    (self._prog.fbr, '', label, -1))
		viter = model.append(last, v)
		vpath = model.get_path(viter)
		self._stack_view.expand_to_path(vpath)
		self._stack_view.scroll_to_cell(vpath)
	    elif ctype == "stack_pop":
		last = self.get_last_iter(model, None)
		# If last is none, that is an error.
		llast = self.get_last_iter(model, last)
		model.remove(llast)
		if model.iter_n_children(last) == 0:
		    model.remove(last)
	    elif ctype == "heap_alloc":
		iter = self.get_block_near_iter(model, change.start)
		row = (change.start, "", change.size, -1)
		if iter is None:
		    ai = model.insert_before(None, None, row)
		else:
		    ai = model.insert_after(None, iter, row)
		for i in [x + change.start for x in range(0, change.size)]:
		    model.append(ai, self.none_value_row(i))
	    elif ctype == "heap_free":
		iter = self.get_block_iter(model, change.start)
		# iter None here is an error
		model.remove(iter)
	    elif ctype == "heap_change" or ctype == "stack_change":
		iter = self.get_block_near_iter(model, change.start)
		base = model.get_value(model.iter_nth_child(iter, 0), 0)
		n = change.start - base
		riter = model.iter_nth_child(iter, n)
		self.set_row_to_value(model, riter, change.start, change.value)

    ### General display updating {{{2
    def reset_memory_display(self):
	self._stack_view.get_model().clear()
	self._heap_view.get_model().clear()

    def update_registers(self):
	self._mc_label.set_text("MC: %d" % self._prog.mc)
	self._lc_label.set_text("LC: %d" % self._prog.lc)
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
		try:
		    self._finished = not self._prog.step()
		    if self._finished:
			self.append_to_console(\
			    "Final Result: %d" % self._prog.stack[0].value)
			self._timer_id = None
			self.display_capture()
			self.clear_temporary_breakpoints()
		    self.update_display()
		except sam.SamError, (errnum,):
		    self._finished = True
		    self._timer_id = None
		    self.clear_temporary_breakpoints()
		    self.append_to_console("Error: %s" % sam.Errors[errnum])
	    return not self._finished

    # Single step, ignores breakpoints
    def single_step(self):
	if not self.is_waiting_for_input() and not self._timer_id:
	    return self.step()

    # Step for while running. Supports breakpoints, etc.
    def run_step(self):
	if self._break_return: # check for nested functions
	    next_asm_st = self.get_current_instruction()[0:3]
	    if next_asm_st == "RST":
		self._break_return = self._break_return - 1
	    elif next_asm_st == "JSR":
		self._break_return = self._break_return + 1
	if not (self._capture is None):
	    prev_code = self.get_current_instruction()
	rv = self.step()
	if not (self._capture is None):
	    self._capture.append(self.capture_current(prev_code))
	if self._prog.lc in self._breakpoints[self._prog.mc]['normal']:
	    return self.pause()
	elif self._prog.lc in self._breakpoints[self._prog.mc]['temporary']:
	    self._breakpoints[self._prog.mc]['temporary'].remove(self._prog.lc)
	    return self.pause()
	elif not (self._break_return is None) and self._break_return == 0:
	    self._break_return = None
	    return self.pause()
	else:
	    return rv

    def pause_no_capture(self):
	if not self.is_waiting_for_input():
	    if self._timer_id:
		gobject.source_remove(self._timer_id)
		self._timer_id = None
		return False

    def pause(self):
	self.pause_no_capture()
	self.display_capture()

    def start(self):
	if not self.is_waiting_for_input():
	    if self._prog and self._timer_id is None:
		if self.run_step():
		    self._timer_id = gobject.timeout_add(\
			self._timeout_interval, self.run_step)
    
    def capture(self):
	if not self.is_waiting_for_input() and self._prog and\
		self._timer_id is None and self._capture is None:
	    self._capture = []
	    self.start()

    def start_pause(self):
	if self._prog:
	    if self._timer_id:
		self.pause()
	    else:
		self.start()

    def reset(self):
	if self._prog:
	    self.pause_no_capture()
	    self._prog.reset()
	    self.update_registers()
	    self.clear_temporary_breakpoints()
	    self.scroll_code_view()
	    self.reset_memory_display()
	    self._finished = False
	    self._timer_id = None
	    self._capture = None
	    self.reset_input_box()
	    self.append_banner_to_console("RESET")

    # GTK handlers {{{3
    def on_step_clicked(self, p):
	self.single_step()

    def on_capture_clicked(self, p):
	self.capture()

    def on_start_activate(self, p):
	self.start()

    def on_pause_activate(self, p):
	self.clear_temporary_breakpoints()
	self.pause()

    def on_start_pause_clicked(self, p):
	self.clear_temporary_breakpoints()
	self.start_pause()
    
    def on_reset_clicked(self, p):
	self.reset()

    ### Breakpoints {{{2
    def toggle_breakpoint(self, module, line):
	if self._breakpoints:
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

    def set_temp_bp(self, module, line):
	if not self._finished:
	    self._breakpoints[module]['temporary'].add(line)
	    self.refresh_code_display()
    
    def step_return(self):
	if self._prog and self._timer_id is None:
	    self._break_return = 1
	    self.start()

    def run_to_line(self, module, line):
	if self._prog and self._timer_id is None:
	    self.set_temp_bp(module, line)
	    self.start()

    def step_over(self):
	if self._prog:
	    self.run_to_line(self._prog.mc, self._prog.lc + 1)

    def clear_normal_breakpoints(self):
	if self._breakpoints:
	    for bps in self._breakpoints:
		for key in bps:
		    if key == "normal":
			bps[key].clear()
	self.refresh_code_display()

    def clear_temporary_breakpoints(self):
	if self._breakpoints:
	    for bps in self._breakpoints:
		for key in bps:
		    if key == "temporary":
			bps[key].clear()
	self._break_return = None
	self.refresh_code_display()

    def on_code_view_row_activated(self, tv, path, col):
	if self._prog:
	    self.toggle_breakpoint(self.get_current_module_num(), path[0])

    def on_toggle_breakpoint_activate(self, p):
	self.toggle_breakpoint_selected()

    def on_remove_all_breakpoints_activate(self, p):
	self.clear_normal_breakpoints()

    # Run menu handlers
    def on_step_over_activate(self, p):
	self.step_over()

    def on_step_return_activate(self, p):
	self.step_return()

    def on_run_to_line_activate(self, p):
	self.run_to_line(self.get_current_module_num(),\
		self.get_selected_code_line())

    ### Capture {{{2
    # Collect data {{{3
    def copy_treestore_level(self, source, sourceIter, target, targetIter):
	sourceChild = source.iter_children(sourceIter)
	while sourceChild:
	    targetC = target.append(targetIter,\
		    tuple([source.get_value(sourceChild, i)\
		    for i in range(0, len(self.none_value_row(0)))]))
	    self.copy_treestore_level(source, sourceChild, target, targetC)
	    sourceChild = source.iter_next(sourceChild)

    def copy_value_treestore(self, model):
	rv = make_value_tree_store()
	self.copy_treestore_level(model, None, rv, None)
	return rv

    def get_previous_instruction(self):
	return self._prog.modules[self._prog.mc].\
		instructions[self._prog.lc-1].assembly

    def get_current_instruction(self):
	return self._prog.modules[self._prog.mc].instructions[self._prog.lc].\
		assembly

    def capture_current(self, prev_code):
	return {'mc': self._prog.mc, 'lc': self._prog.lc,\
		'fbr': self._prog.fbr, 'sp': self._prog.sp,\
		'code': prev_code,\
		'stack': self.copy_value_treestore(\
		    self._stack_view.get_model()),\
		'heap': self.copy_value_treestore(self._heap_view.get_model())}

    # Display data {{{3
    def display_capture(self):
	if not (self._capture is None):
	    capture_disp = Capture(self._capture)
	    self._capture = None
	    capture_disp.show()

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
    def append_to_console(self, str, show_time=True):
	buf = self._console.get_buffer()
	if buf.get_char_count() == 0:
	    st = ""
	else:
	    st = "\n"
	if show_time:
	    buf.insert(buf.get_end_iter(),\
		    "%s[%s] %s" % (st, time.strftime('%X'), str))
	else:
	    buf.insert(buf.get_end_iter(),\
		    "%s%s" % (st, str))
	self._console.scroll_mark_onscreen(self._console_end_mark)

    def append_banner_to_console(self, str):
	dashes = "-----------------------------------"
	self.append_to_console("%s %s %s" % (dashes, str, ""), False)

    # on_clear_console {{{3
    def on_clear_console_activate(self, p):
	self._console.get_buffer().set_text("")

    ### Speed change handlers {{{2
    # Requires: newSpeed >= 0
    def change_speed(self, newSpeed):
	self._timeout_interval = newSpeed
	if self._prog and self._timer_id:
	    self.pause_no_capture()
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

    # About dialog {{{2
    def on_about_activate(self, p):
	self._about_dialog.show()

    def on_about_close(self, p, res):
	self._about_dialog.hide()

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
