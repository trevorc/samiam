#!/usr/bin/env python

import pygtk
import gtk, gtk.glade

#import libsam

class GSam:
    def __init__(self):
	self._xml = gtk.glade.XML('gsam.glade')
	self._xml.signal_autoconnect(self)

	self._open_dialog = self._xml.get_widget('filechooser')
	self._file = None

    def _filechooser_factory(self, title, p):
	pass

    def on_open_activate(self, p):
	if self._file:
	    self._file.close()
	filechooser = self._filechooser_factory(p)

    def gtk_main_quit(*self):
	gtk.main_quit()

if __name__ == '__main__':
    gsam = GSam()

    gtk.main()
