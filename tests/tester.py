#!/usr/bin/env python2.5

import os
import sqlite3 as sqlite

class Tester:
    def __init__(self, cmd, db, path=None, libpath=None, infcmd=None):
	import os

	self._cmd = cmd
	self._db = self._read_db(db)

	if path:
	    os.putenv('PATH', path)
	if libpath:
	    os.putenv('LD_LIBRARY_PATH', libpath)
	if infcmd:
	    self._inf = os.system(infcmd)

    def _read_db(self, db):
	try:
	    conn = sqlite.connect(db)
	    cursor = conn.cursor()
	    cursor.execute('select * from tests')
	    return cursor.fetchall()
	finally:
	    conn.close()

    def _run(self, test, input):
	return None, None

    def testall(self):
	failed = []

	# parallelize me
	for test, status, input, output in self._db:
	    actual_status, actual_output = self._run(test, input)

if __name__ == '__main__':
    tester = Tester(cmd='samiam', db='tests.db',
		    path='../build/samiam', libpath='../build/libsam',
		    infcmd='./inf')
    tester.testall()
