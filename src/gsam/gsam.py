#!/usr/bin/env python

import pygtk
import gtk, gtk.glade, gobject
import sam

class CodeTreeModel(gtk.GenericTreeModel):
    column_types = (str, int, str, str, str) # TODO Last two may become images
    column_names = ['Current', 'Line', 'Breakpoint', 'Code', 'Labels']

    def __init__(self, prog, modnum):
	gtk.GenericTreeModel.__init__(self)
	self._prog = prog
	self._module_n = modnum
	# TODO multi-module support
	self._instructions = self._prog.instructions
    
    def on_get_flags(self):
	return gtk.TREE_MODEL_LIST_ONLY | gtk.TREE_MODEL_ITERS_PERSIST
    
    def on_get_n_columns(self):
	return len(self.column_types)

    def on_get_column_type(self, n):
	return self.column_types[n]

    # TODO What is this method?
    def on_get_iter(self, path):
	return path[0]

    # TODO What is this method?
    def on_get_path(self, rowref):
	return rowref

    def on_get_value(self, rowref, column):
	item = self._instructions[rowref]
	if column is 0:
	    # TODO multi-module support
	    # TODO deuglify
	    if self._prog.pc == rowref:
		return "-->"
	    else:
		return ""
	elif column is 1:
	    return rowref
	elif column is 2:
	    return "" # TODO breakpoints
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

    # TODO What is this method?
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

class GSam:
    def __init__(self):
	self._xml = gtk.glade.XML('gsam.glade')
	self._xml.signal_autoconnect(self)

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
	self._show_heap = self._xml.get_widget('show_heap')

	self._pc_label = self._xml.get_widget('pc_label')
	self._fbr_label = self._xml.get_widget('fbr_label')
	self._sp_label = self._xml.get_widget('sp_label')

	self._console = self._xml.get_widget('console_view')

	self._file = None
	self._prog = None

	column_names = ['Current', 'Line', 'Breakpoint', 'Code', 'Labels']
	for n in range(0, len(column_names)):
	    cell = gtk.CellRendererText()
	    tvcolumn = gtk.TreeViewColumn(column_names[n], cell, text=n)
	    self._code_view.append_column(tvcolumn)
	self._code_view.set_search_column(3)

	column_names = ['Address', 'Type', 'Value']
	for n in range(0, len(column_names)):
	    renderer = gtk.CellRendererText()
	    column = gtk.TreeViewColumn(column_names[n], renderer, text=n)
	    self._stack_view.append_column(column)
	self._stack_view.set_search_column(1)

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

    def on_close_activate(self, p):
	if self._file:
	    self._file = None
	    self._prog = None
	self._main_window.set_title(self._default_title)
	self._code_view.set_model(None)
	self._stack_view.set_model(None)
	self._heap_view.set_model(None)
	self._code_models = None

    def on_open_activate(self, p):
	filechooser = self._filechooser_factory("Select a SaM file", p)
	response = filechooser.run()
	if response == gtk.RESPONSE_OK:
	    # close old file
	    self.on_close_activate(p)
	    self.init_program_display(filechooser.get_filename())
	filechooser.destroy()

    def get_n_modules(self):
	return len(self._prog.modules)

    # Note: current module = one being displayed to the user,
    #	not one being executed
    def get_current_module_num(self):
	return 0

    def get_current_module(self):
	return self._prog.modules[self.get_current_module_num()]

    def get_current_code_model(self):
	return self._code_models[self.get_current_module_num()]

    def set_current_code_model(self, model):
	self._code_models[self.get_current_module_num()] = model

    def update_code_display(self):
	if self.get_current_code_model() is None:
	    # TODO should be _prog.modules[n].instructions
	    self.set_current_code_model(CodeTreeModel(self._prog, \
		    self.get_current_module()))
	self._code_view.set_model(self.get_current_code_model())

    # TODO
    def reset_memory_display(self):
	self._stack_view.set_model(gtk.ListStore(gobject.TYPE_LONG,\
	    gobject.TYPE_STRING, gobject.TYPE_LONG))
	self._heap_view.set_model(None) # TODO heap

    def init_program_display(self, filename):
	self._file = filename
	self._main_window.set_title("%(title)s - %(file)s" %
		{'title': self._default_title, 'file': filename})
	self._prog = sam.Program(filename)
	self._finished = False

	self._code_models = [None for i in range(0, self.get_n_modules())]
	self.update_code_display()
	self.reset_memory_display()

    def update_registers(self):
	self._pc_label.set_text("PC: %d" % self._prog.pc)
	self._fbr_label.set_text("FBR: %d" % self._prog.fbr)
	self._sp_label.set_text("SP: %d" % self._prog.sp)

    def scroll_code_view(self):
	if self._prog.mc == self.get_current_module_num():
	    model = self._code_view.get_model()
	    nIter = model.iter_nth_child(None, self._prog.pc)
	    nPath = model.get_path(nIter)
	    self._code_view.scroll_to_cell(nPath)
	    self._code_view.queue_draw() # TODO better way to do this?

    def value_to_row(self, addr, v):
	return (addr, sam.TypeChars[v.type], v.value)

    def set_row_to_value(self, model, iter, v):
	row = self.value_to_row(model.get_path(iter)[0], v)
	for i in range(0, len(row)):
	    model.set(iter, i, row[i])

    def get_nth_iter(self, model, n):
	return model.iter_nth_child(None, n)

    def get_nth_path(self, model, n):
	return model.get_path(self.get_nth_iter(model, n))

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
		self.set_row_to_value(model, iter, change.value)

    def update_display(self):
	self.update_registers()
	self.scroll_code_view()
	self.handle_changes()

    def on_step_clicked(self, p):
	if self._prog:
	    if not self._finished:
		self._finished = not self._prog.step()
		if self._finished:
		    self.append_to_console(\
			    "Final Result: %d" % self._prog.stack[0].value)
		self.update_display()
    
    def on_reset_clicked(self, p):
	if self._prog:
	    self._prog.reset()
	    self.update_registers()
	    self.scroll_code_view()
	    self.reset_memory_display()
	    self._finished = False
    
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
    
    def append_to_console(self, str):
	buf = self._console.get_buffer()
	buf.insert(buf.get_end_iter(), "%s\n" % str)

    def gtk_main_quit(*self):
	gtk.main_quit()

if __name__ == '__main__':
    gsam = GSam()

    gtk.main()
