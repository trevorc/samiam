#!/usr/bin/env python

# imports {{{1
import pygtk
import gtk, gtk.glade, gobject
import sam
import time
import sys
import os
import pango
import re

# prepare_value_view {{{1
# make_value_tree_store () {{{2
def make_value_tree_store(stack):
    if stack:
	first = gobject.TYPE_LONG
    else:
	first = gobject.TYPE_STRING
    return gtk.TreeStore(first,
			 gobject.TYPE_STRING,
			 gobject.TYPE_STRING,
			 gobject.TYPE_LONG)

# prepare_value_view () {{{2
def prepare_value_view(view, stack=True):
    type_colors = {
	'none':  ('lightgray'),
	'int':   ('white'),
	'float': ('yellow'),
	'sa':    ('pink'),
	'ha':    ('purple'),
	'pa':    ('green'),
	'block': ('black')
    }

    def mem_color_code(column, cell, model, iter):
	type_num = model.get_value(iter, 3)
	if type_num in range(0, len(sam.Types)):
	    row_color = type_colors[sam.Types[type_num]]
	    cell.set_property('foreground', 'black')
	else:
	    row_color = type_colors['block']
	    cell.set_property('foreground', 'white')
	cell.set_property('cell-background', row_color)

    column_names = ('Address', 'Type', 'Value')
    view.set_model(make_value_tree_store(stack))
    for n in range(0, len(column_names)):
	renderer = gtk.CellRendererText()
	column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	column.set_cell_data_func(renderer, mem_color_code);
	view.append_column(column)
    view.set_search_column(1)

# class CodeTreeModel {{{1
class CodeTreeModel(gtk.GenericTreeModel):
    column_types = (str, int, gtk.gdk.Pixbuf, str, str)
    column_names = ('Current', 'Line', 'Breakpoint', 'Code', 'Labels')

    def __init__(self, prog, modnum, bp, bpnorm, bptemp):
	gtk.GenericTreeModel.__init__(self)
	self._prog = prog
	self._module_n = modnum
	self._instructions = prog.modules[modnum].instructions
	self._breakpoints = bp[modnum]

	self._blank_pixbuf = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB,
					    True,
					    8,
					    bpnorm.get_width(),
					    bpnorm.get_height())
	self._blank_pixbuf.fill(0x00000000)
	self._bp_norm_pixbuf = bpnorm
	self._bp_temp_pixbuf = bptemp

    def _format_labels(self, labels):
	format = '(' + labels[0]
	for i in labels[1:]:
	    format += ', ' + i
	format += ')'

	return format

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
		return self._format_labels(item.labels)
	    else:
		return ""
	# and if the column is invalid... (error: it never happens)
	else:
	    return "%(a)s (Error: %(c)d)" % {
		'a': item.assembly,
		'c': column
	    }
    
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

	widget_prefixes = (
	    'inst',
	    'pc_label',
	    'fbr_label',
	    'sp_label',
	    'stack_view',
	    'heap_view'
	)
	self._widgets = [
	    map(self._xml.get_widget,
		map(lambda x: "%s%d" % (x, i), widget_prefixes))
	    for i in range(1, 6)
	]
	map(prepare_value_view,
	    map(self._xml.get_widget, [
		"stack_view%d" % i for i in range(1, 6)
	    ]))

	self._data = data

	for i in range(0, len(self._widgets)):
	    if i < len(data):
		self.show_data_at(data[i], i)

	# Setup instructions view {{{4
	column_names = ('Addr', 'Code')
	self._instructions_view.set_model(gtk.ListStore(str, str))
	for n in range(0, len(column_names)):
	    renderer = gtk.CellRendererText()
	    column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	    self._instructions_view.append_column(column)
	self._instructions_view.set_search_column(1)
	
	# Insert data into instructions view {{{4
	for row in data:
	    self._instructions_view.get_model().append(("%i:%i" %
							(row['mc'],
							 row['lc']),
							 row['code']))

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
	self._show_heap = self._xml.get_widget('show_heap')

	self._mc_label = self._xml.get_widget('mc_label')
	self._lc_label = self._xml.get_widget('lc_label')
	self._fbr_label = self._xml.get_widget('fbr_label')
	self._sp_label = self._xml.get_widget('sp_label')

	self._console = self._xml.get_widget('console_view')

	self._input_box = self._xml.get_widget('program_entry_box')
	self._input = self._xml.get_widget('program_entry')
	self._input_canceled = False

	self._custom_dialog = self._xml.get_widget('custom_dialog')
	self._custom_entry = self._xml.get_widget('custom_entry')
	self._custom_speed = self._xml.get_widget('custom_speed')

	# Prepare console tags
	self._console.get_buffer().create_tag('bold', weight=pango.WEIGHT_BOLD)

	# Setup custom speed dialog {{{4
	self._custom_dialog.add_buttons(gtk.STOCK_CANCEL,
					gtk.RESPONSE_REJECT,
					gtk.STOCK_OK,
					gtk.RESPONSE_ACCEPT)

	# Set default values {{{4
	self._file = None
	self._prog = None
	self._breakpoints = None
	self._break_return = None
	self._capture = None

	self._timer_id = None
	self._speeds = {'full': 0, 'fast': 10, 'medium': 100, 'slow': 1000}
	self._timeout_interval = self._speeds['fast']

	self._console_end_mark = self._console.get_buffer().create_mark("end",
	    self._console.get_buffer().get_end_iter(), False)

	self._module_combobox.set_model(gtk.ListStore(str))
	cell = gtk.CellRendererText()
	self._module_combobox.pack_start(cell, True)
	self._module_combobox.add_attribute(cell, 'text', 0)

	self.init_prepare_inputs_window()

	# Code display setup {{{4
	# TODO better pixbufs?
	self._bp_norm = self._main_window.render_icon(gtk.STOCK_NO,
						      gtk.ICON_SIZE_MENU)
	self._bp_temp = self._main_window.render_icon(gtk.STOCK_YES,
						      gtk.ICON_SIZE_MENU)

	column_names = ('Current', 'Line', 'Breakpoint', 'Code', 'Labels')

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
	prepare_value_view(self._stack_view, True)
	prepare_value_view(self._heap_view, False)

	# Handle file input if given {{{4
	if filename:
	    self.open_file(filename)

    ### Open/close file methods {{{2
    # _filechooser_factory () {{{3
    def _filechooser_factory(self, title, p):
	dialog = gtk.FileChooserDialog(title=title,
				       parent=self._main_window,
				       buttons=(gtk.STOCK_CANCEL,
						gtk.RESPONSE_CANCEL,
						gtk.STOCK_OPEN,
						gtk.RESPONSE_OK))
	dialog.set_default_response(gtk.RESPONSE_OK)

	filter = gtk.FileFilter()
	filter.set_name("sam files")
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
	    self.append_banner_to_console("Closed %s." % self._file)
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
	self.reset_input_box('close')

    # on_close_activate () {{{3
    def on_close_activate(self, p):
	self.close_file()

    # open_file () {{{3
    def open_file(self, filename):
	# close old file
	if self._file:
	    self.close_file()
	self.init_program_display(filename)

    # on_open_activate () {{{3
    def on_open_activate(self, p):
	filechooser = self._filechooser_factory("Select a sam file", p)
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
	    self._breakpoints = [
		{
		    'normal': set(),
		    'temporary': set()
		}
		for i in range(0, self.get_n_modules())
	    ]
	    self._code_models = [None for i in range(0, self.get_n_modules())]
	    self.update_code_display()
	    self.reset_memory_display()

	    self._main_window.set_title("%(title)s - %(file)s" %
		    {'title': self._default_title, 'file': filename})
	    self.append_banner_to_console("Opened %s." % filename)
	except sam.ParseError:
	    self._file = None
	    self._prog = None
	    self.append_to_console("Parse error loading file %s" % filename,
				   False)

    ### Code display handling {{{2
    def get_selected_code_line(self):
	iter = self._code_view.get_selection().get_selected()[1]
	if iter:
	    return self._code_view.get_model().get_path(iter)[0]
	else:
	    return None

    def set_selected_code_line(self, line):
	self._code_view.get_selection().select_path((line,))
	self._code_view.scroll_to_cell((line,))
	self.refresh_code_display()

    def refresh_code_display(self):
	self._code_view.queue_draw() # TODO better way to do this?

    def get_n_modules(self):
	return len(self._prog.modules)

    # Note: current module = one being displayed to the user,
    #	not one being executed
    def get_current_module_num(self):
	return self._module_combobox.get_active()

    def set_current_module_num(self, modnum):
	if self._prog and self._code_models:
	    self._module_combobox.set_active(modnum)
	    self.update_code_display()

    def get_current_module(self):
	return self._prog.modules[self.get_current_module_num()]

    def get_current_code_model(self):
	return self._code_models[self.get_current_module_num()]

    def set_current_code_model(self, model):
	self._code_models[self.get_current_module_num()] = model

    def update_code_display(self):
	if self.get_current_code_model() is None:
	    self.set_current_code_model(CodeTreeModel(self._prog,
					self.get_current_module_num(),
					self._breakpoints,
					self._bp_norm,
					self._bp_temp))
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
    def format_addr(self, addr):
	if type(addr).__name__ == 'tuple':
	    return "%i:%i" % addr
	else:
	    return addr

    def value_to_row(self, addr, v):
	vtype = sam.Types[v.type]
	if vtype == 'none':
	    vstr = "-"
	elif vtype == 'pa' or vtype == 'ha':
	    vstr = "%i:%i" % v.value
	else:
	    vstr = "%i" % v.value
	return (self.format_addr(addr), sam.TypeChars[v.type], vstr, v.type)

    def none_value_row(self, addr):
	return (self.format_addr(addr), 'N', '0', 0)

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

    def get_heap_iter(self, model, block):
	iter = model.get_iter_root()
	if iter is None:
	    return None
	while iter and int(model.get_value(iter, 0)) <= block:
	    piter = iter
	    iter = model.iter_next(iter)
	if int(model.get_value(piter, 0)) == block:
	    return piter
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
	    # stack_push {{{4
	    if ctype == "stack_push":
		v = self.value_to_row(change.start, change.value)
		last = self.get_last_iter(model, None)
		if last is None:
		    fbr = None
		else:
		    fbr = model.get_value(last, 0)
		if last is None or fbr is None or fbr < self._prog.fbr:
		    insts = self._prog.modules[self._prog.mc].instructions
		    prev = insts[self._prog.lc - 1]
		    if len(prev.labels) == 0:
			plabel = None
		    else:
			plabel = prev.labels[0]
		    next = insts[self._prog.lc].assembly
		    if next[0:3] == "JSR":
			nlabel = next[4:]
		    else:
			nlabel = None
		    if nlabel and not plabel:
			label = nlabel
		    elif plabel and not nlabel:
			label = plabel
		    else:
			label = None

		    last = model.append(None,
					(self._prog.fbr,
					 '',
					 label,
					 -1))
		viter = model.append(last, v)
		vpath = model.get_path(viter)
		self._stack_view.expand_to_path(vpath)
		self._stack_view.scroll_to_cell(vpath)
	    # stack_pop {{{4
	    elif ctype == "stack_pop":
		last = self.get_last_iter(model, None)
		# If last is none, that is an error.
		llast = self.get_last_iter(model, last)
		model.remove(llast)
		if model.iter_n_children(last) == 0:
		    model.remove(last)
	    # heap_alloc {{{4
	    elif ctype == "heap_alloc":
		iter = self.get_block_near_iter(model, change.start)
		row = (change.start[0], "", change.size, -1)
		if iter is None:
		    ai = model.insert_before(None, None, row)
		else:
		    ai = model.insert_after(None, iter, row)
		for i in [x for x in range(0, change.size)]:
		    model.append(ai, self.none_value_row((change.start[0], i)))
	    # heap_free {{{4
	    elif ctype == "heap_free":
		iter = self.get_heap_iter(model, change.start[0])
		# iter None here is an error
		model.remove(iter)
	    # heap_change {{{4
	    elif ctype == "heap_change":
		iter = self.get_heap_iter(model, change.start[0])
		riter = model.iter_nth_child(iter, change.start[1])
		self.set_row_to_value(model, riter, change.start, change.value)
	    # stack_change {{{4
	    elif ctype == "stack_change":
		iter = self.get_block_near_iter(model, change.start)
		base = model.get_value(model.iter_nth_child(iter, 0), 0)
		n = change.start - base
		riter = model.iter_nth_child(iter, n)
		self.set_row_to_value(model, riter, change.start, change.value)

    ### Stack/heap value target jumping {{{2
    def on_mem_view_row_activated(self, tview, path, col):
	model = tview.get_model()
	iter = model.get_iter(path)
	typec = model.get_value(iter, 1)
	val = model.get_value(iter, 2)
	if typec == 'S' or typec == 'H':
	    addr = int(val)
	    if typec == 'S':
		tv = self._stack_view
	    elif typec == 'H':
		tv = self._heap_view
	    model = tv.get_model()
	    baseIter = self.get_block_near_iter(model, addr)
	    basePath = model.get_path(baseIter)
	    baseAddr = model.get_value(model.iter_nth_child(baseIter, 0), 0)
	    n = addr - baseAddr
	    if n < model.iter_n_children(baseIter):
		riter = model.iter_nth_child(baseIter, n)
		rpath = model.get_path(riter)

		tv.expand_row(basePath, False)
		tv.get_selection().select_path(rpath)
		tv.scroll_to_cell(rpath)
		tv.queue_draw()
	elif typec == 'P':
	    pa = val.split(':')
	    self.set_current_module_num(int(pa[0]))
	    self.set_selected_code_line(int(pa[1]))

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
			self.append_to_console(
			    "Final Result: %d" % self._prog.stack[0].value)
			self._timer_id = None
			self.display_capture()
			self.clear_temporary_breakpoints()
		    self.update_display()
		except IndexError:
		    # TODO: what should we do here?
		    self.append_to_console(
			"warning: terminated with empty stack")
		    return False
		except sam.SamError, (errnum,):
		    error = sam.Errors[errnum]
		    if self._input_canceled and error == "I/O error":
			# Not really an error, user canceled the input
			return False
		    else:
			self._finished = True
			self._timer_id = None
			self.clear_temporary_breakpoints()
			self.append_to_console("Error: %s" % error)
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
	# if it got closed on an input...
	if self._prog:
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
	else:
	    return False

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
		    self._timer_id = gobject.timeout_add(
			self._timeout_interval,
			self.run_step)
    
    def capture(self):
	if not self.is_waiting_for_input() and \
	   self._prog and \
	   self._timer_id is None and \
	   self._capture is None:
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
	    self.reset_input_box('reset')
	    self.append_banner_to_console("Reset.")

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
	    self.toggle_breakpoint(self.get_current_module_num(),
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
	self.run_to_line(self.get_current_module_num(),
			 self.get_selected_code_line())

    ### Capture {{{2
    # Collect data {{{3
    def copy_treestore_level(self, source, sourceIter, target, targetIter):
	sourceChild = source.iter_children(sourceIter)
	while sourceChild:
	    targetC = target.append(targetIter, tuple([
					source.get_value(sourceChild, i)
					for i in range(
					    0, len(self.none_value_row(0)))
				    ]))
	    self.copy_treestore_level(source, sourceChild, target, targetC)
	    sourceChild = source.iter_next(sourceChild)

    def copy_value_treestore(self, stack, model):
	rv = make_value_tree_store(stack)
	self.copy_treestore_level(model, None, rv, None)
	return rv

    def get_previous_instruction(self):
	return self._prog.modules[self._prog.mc]. \
		instructions[self._prog.lc-1].assembly

    def get_current_instruction(self):
	return self._prog.modules[self._prog.mc].instructions[self._prog.lc]. \
	    assembly

    def capture_current(self, prev_code):
	return {
	    'mc': self._prog.mc,
	    'lc': self._prog.lc,
	    'fbr': self._prog.fbr,
	    'sp': self._prog.sp,
	    'code': prev_code,
	    'stack': self.copy_value_treestore(True,
		self._stack_view.get_model()),
	    'heap': self.copy_value_treestore(False,
		self._heap_view.get_model())
	}

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
    def append_to_console(self, str, show_time=True, tag=None):
	buf = self._console.get_buffer()
	if buf.get_char_count() == 0:
	    st = ""
	else:
	    st = "\n"
	if show_time:
	    timestamp = '[%s] ' % time.strftime('%X')
	    message = re.sub('\n', '\n%s' % timestamp, str.rstrip())
	    text = '%s%s%s' % (st, timestamp, message)
	else:
	    text = "%s%s" % (st, str)
	if tag:
	    buf.insert_with_tags_by_name(buf.get_end_iter(),
					 text,
					 tag)
	else:
	    buf.insert(buf.get_end_iter(), text)
	self._console.scroll_mark_onscreen(self._console_end_mark)

    def append_banner_to_console(self, str):
	self.append_to_console(str, False, 'bold')

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
	return self._input_box.get_property("visible")

    def reset_input_box(self, reason=None):
	if self.is_waiting_for_input():
	    self._input_ready = False
	    self._input_box.hide_all()
	    self._input_canceled = reason
	    gtk.main_quit()

    def sam_string_input(self):
	# If we have a prepared input ready, just go ahead and use it
	if self.is_input_auto():
	    auto_in = self.prepared_input_next()
	    if not (auto_in is None):
		self.append_to_console("Prepared Input: %s" % auto_in)
		return auto_in

	self._input.set_text("")
	self._input_ready = False
	self._input_canceled = False
	self._input_box.show_all()
	self._input.grab_focus() # Hmm... focus grabbing...
	gtk.main()
	if self._input_ready:
	    res = self._input.get_text()
	    self._input.set_text("")
	    self._input_box.hide_all()
	    self.append_to_console("User Input: %s" % res)
	    return res
	elif self._input_canceled == 'quit': # Input canceled
	    self.gtk_main_quit()
	else:
	    return None

    def on_program_entry_activate(self, p):
	self._input_ready = True
	gtk.main_quit()

    def sam_print(self, str, stream):
	if stream == 'stdout':
	    stream_prefix = 'Program output: '
	else:
	    stream_prefix = ''
	self.append_to_console('%s%s' % (stream_prefix, str))

    ### Prepared Inputs {{{2
    # Init {{{3
    def init_prepare_inputs_window(self):
	self._prepare_inputs_window = self._xml.get_widget(
		'prepare_inputs_window')
	self._inputs_view = self._xml.get_widget('inputs_view')
	self._input_entry = self._xml.get_widget('input_entry')
	self._input_auto = self._xml.get_widget('input_auto')
	self._inputs_index = 0

	self._prepare_inputs_value_dialog = self._xml.get_widget(
	    'prepare_inputs_value_dialog')
	self._prepare_inputs_value_dialog.add_buttons(
	    gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
	    gtk.STOCK_OK, gtk.RESPONSE_ACCEPT)

	self._inputs_view.set_model(gtk.ListStore(str))
	cell = gtk.CellRendererPixbuf()
	def inputs_curr(column, cell, model, iter, self):
	    i = self.get_input_curr_index()
	    if not (i is None) and i == model.get_path(iter)[0]:
		stock = gtk.STOCK_GO_FORWARD
	    else:
		stock = None
	    cell.set_property('stock_id', stock)
	self._inputs_view.insert_column_with_data_func(-1, "curr", cell,
	    inputs_curr, self)
	cell = gtk.CellRendererText()
	cell.set_property('editable', True)
	cell.connect('edited', self.inputs_cell_edited)
	column = gtk.TreeViewColumn("input", cell, text=0)
	self._inputs_view.append_column(column)
	self._inputs_view.set_search_column(0)

    def inputs_cell_edited(self, cell, path, new_text):
	model = self._inputs_view.get_model()
	iter = model.get_iter(path)
	model.set_value(iter, 0, new_text)

    # Helpers {{{3
    def get_input_curr_index(self):
	return self._inputs_index

    def is_input_auto(self):
	return self._input_auto.get_active()

    def get_inputs_focus_path(self):
	focus = self._inputs_view.get_cursor()[0]
	if focus is None:
	    return None
	else:
	    return focus

    def get_inputs_focus_iter(self):
	path = self.get_inputs_focus_path()
	if path is None:
	    return None
	else:
	    return self._inputs_view.get_model().get_iter(path)

    def get_inputs_focus_row(self):
	path = self.get_inputs_focus_path()
	if path is None:
	    return None
	else:
	    return path[0]

    # GUI/Editing data {{{3
    def show_prepare_inputs_window(self):
	self._prepare_inputs_window.show()

    def hide_prepare_inputs_window(self):
	self._prepare_inputs_window.hide()

    def show_add_input_dialog(self):
	self._input_entry.set_text('')
	self._input_entry.grab_focus()
	res = self._prepare_inputs_value_dialog.run()
	self._prepare_inputs_value_dialog.hide()
	if res == gtk.RESPONSE_ACCEPT:
	    self._inputs_view.get_model().append(
		    (self._input_entry.get_text(),))

    # Using data {{{3
    # iterates through prepared inputs
    def prepared_input_next(self):
	i = self.get_input_curr_index()
	model = self._inputs_view.get_model()
	if not (i is None) and i < model.iter_n_children(None):
	    j = i + 1
	    if j < model.iter_n_children(None):
		self._inputs_index = j
	    else:
		self._inputs_index = None
	    return model.get_value(model.iter_nth_child(None, i), 0)
	return None

    def use_prepared_input(self):
	if self.is_waiting_for_input():
	    input = self.prepared_input_next()
	    if input is None:
		return False
	    else:
		self._input.set_text(input)
		self.on_program_entry_activate(None)
		# Note: this will not return
	else:
	    return False

    # GTK handlers {{{3
    def on_input_entry_activate(self, p):
	self._prepare_inputs_value_dialog.response(gtk.RESPONSE_ACCEPT)

    def on_show_prepare_inputs_activate(self, p):
	self.show_prepare_inputs_window()

    def on_input_close_clicked(self, p):
	self.hide_prepare_inputs_window()

    def on_input_add_clicked(self, p):
	self.show_add_input_dialog()

    def on_input_remove_clicked(self, p):
	model = self._inputs_view.get_model()
	model.remove(self.get_inputs_focus_iter())
	cur_index = self.get_input_curr_index()
	if cur_index >= model.iter_n_children(None) and cur_index > 0:
	    self._inputs_index = self._inputs_index - 1

    def on_input_clear_clicked(self, p):
	self._inputs_index = 0
	self._inputs_view.get_model().clear()

    def on_inputs_view_row_activated(self, treeview, path, column):
	self._inputs_index = path[0]
	self.hide_prepare_inputs_window()
	self.use_prepared_input()

    # About dialog {{{2
    def on_about_activate(self, p):
	self._about_dialog.show()

    def on_about_close(self, p, res):
	self._about_dialog.hide()

    # gtk_main_quit () {{{2
    def gtk_main_quit(*self):
	self[0].reset_input_box('quit')
	self[0].close_file()
	gtk.main_quit()

# main {{{1
if __name__ == '__main__':
    if len(sys.argv) > 1:
	gsam = GSam(sys.argv[1])
    else:
	gsam = GSam()
	gsam.on_open_activate(None)

    gtk.main()
