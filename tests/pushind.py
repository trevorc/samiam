# $Id$
#
# pushind.py - benchmark the sam allocator
#
# $Log$
# Revision 1.1  2006/12/15 06:50:04  trevor
# Add the allocator benchmark.
#

num = 32769
#num = 840

for i in range(num):
    print 'PUSHIMM 1'
    print 'MALLOC'
    print 'DUP'
    print 'PUSHIMM 1'
    print 'STOREIND'
print 'DUP'
print 'PUSHIND'
print 'SWAP'
print 'FREE'
print 'SWAP'
for i in range(num - 2):
    print 'DUP'
    print 'PUSHIND'
    print 'SWAP'
    print 'FREE'
    print 'ADD'
    print 'SWAP'
print 'DUP'
print 'PUSHIND'
print 'SWAP'
print 'FREE'
print 'ADD'
print 'STOP'

