#!/usr/bin/env python

import pygtk
import gtk, gtk.glade
import sam

#import libsam

class GSam:
    def __init__(self):
	self._xml = gtk.glade.XML('gsam.glade')
	self._xml.signal_autoconnect(self)

	self._module_combobox = self._xml.get_widget('module_combobox')
	self._code_view = self._xml.get_widget('code_view')
	self._stack_view = self._xml.get_widget('stack_view')
	self._heap_view = self._xml.get_widget('heap_view')
	self._file = None

    def _filechooser_factory(self, title, p):
	dialog = gtk.FileChooserDialog(title=title, \
		parent=self._xml.get_widget('main_window'),\
		buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,\
		    gtk.STOCK_OPEN,gtk.RESPONSE_OK))
	dialog.set_default_response(gtk.RESPONSE_OK)

	filter = gtk.FileFilter()
	filter.set_name("All files")
	filter.add_pattern("*")
	dialog.add_filter(filter)

	filter = gtk.FileFilter()
	filter.set_name("SaM files")
	filter.add_pattern("*.sam")
	dialog.add_filter(filter)

	return dialog

    def on_open_activate(self, p):
	filechooser = self._filechooser_factory("Select a SaM file", p)
	response = filechooser.run()
	if response == gtk.RESPONSE_OK:
	    if self._file:
		self._file.close()
	    init_program_display(filechooser.get_filename())
	filechooser.destroy()

    def update_code_display(self)
	pass
#	TODO self._code_view

    def init_program_display(self, filename)
	self._file = filename
	self._prog = sam.Program(filename)

    def gtk_main_quit(*self):
	gtk.main_quit()

if __name__ == '__main__':
    gsam = GSam()

    gtk.main()
