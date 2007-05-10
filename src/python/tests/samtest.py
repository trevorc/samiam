#!/usr/bin/env python

import sam
import sys

prog = sam.Program(file = sys.argv[1])
#for module in prog.modules:
#    print help(module)

while prog.step():
    pass
#    if len(prog.stack) > 0:
#	print help(prog.stack[0])
#    print "PC:", prog.pc, " -- instruction: '" , \
#	    prog.modules[0], \
#	    "'", "-- Stack size:", len(prog.stack)
#    for val in prog.stack:
#	print "PC:", prog.pc, "-- Stack value type:", sam.Types[val.type], \
#		"value:", val.value
#    pass # print prog.pc
print "Final PC:", prog.pc
if len(prog.stack) == 0:
    print "Error: empty stack at end of SaM program"
else:
    print "Return Value:", prog.stack[0].value
