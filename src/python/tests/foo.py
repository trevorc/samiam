import sam
import sys
es = sam.ExecutionState(sys.argv[1])
print 'fbr: %d' % es.fbr
print 'pc:  %d' % es.pc
print 'sp:  %d' % es.sp
print 'bt:  %d' % es.bt
es.bt = True
print 'bt:  %d' % es.bt

for i in es:
    print i
