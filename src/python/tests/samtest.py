#!/usr/bin/env python

import sam
import sys

prog = sam.Program(file = sys.argv[1])
print "dir(sam):", dir(sam)
print "dir(sam.?)", dir(sam.Program)
#for module in prog.modules:
#    print module.get_foo()

while prog.step():
#    if len(prog.stack) > 0:
#	print help(prog.stack[0])
    print "PC:", prog.pc, " -- instruction: '" , \
	    prog.instructions[prog.pc].assembly, \
	    "'", "-- Stack size:", len(prog.stack)
#    for val in prog.stack:
#	print "PC:", prog.pc, "-- Stack value type:", sam.Types[val.type], \
#		"value:", val.value
#    pass # print prog.pc
print "Final PC:", prog.pc
if len(prog.stack) == 0:
    print "Error: empty stack at end of SaM program"
else:
    print "Return Value:", prog.stack[0].value
