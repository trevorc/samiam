#!/usr/bin/env python

sam_sources = {
    'libsam': [
	'array',
	'error',
	'es',
	'execute',
	'execute_types',
	'hash_table',
	'io',
	'main',
	'opcode',
	'parse',
	'string',
	'util',
    ],
    'samiam': [
	'samiam',
    ],
}

cflags = {
    'standards': [
	'-std=c99',
    ],
    'warnings': [
	'-W' + flag for flag in [
	    '',
	    'error',
	    'all',
	    'missing-prototypes',
	    'missing-declarations',
	    'strict-prototypes',
	    'pointer-arith',
	    'nested-externs',
	    'disabled-optimization',
	    'undef',
	    'endif-labels',
	    'shadow',
	    'cast-align',
	    'strict-aliasing=2',
	    'write-strings',
	    'missing-noreturn',
	    'missing-format-attribute',
	    'redundant-decls',
	    'format',
	]
    ],
    'optimizations': [
	'-pipe',
#	'-O3',
    ],
    'machine': [
#	'-msse3 -march=pentium-m',
#	'-mpower -mpowerpc -mcpu=powerpc', 
#	'-mthumb-interwork -msoft-float',
    ],
    'debug': [
	'-g',
	'-ggdb'
    ],
}

def transform(dir):
    return [dir + '/' + file + '.c' for file in sam_sources[dir]]


env = Environment(CC='gcc',
		  CCFLAGS=' '.join(sum([v for k,v in cflags.items()], [])),
		  CPPPATH='include')

env.SharedLibrary('libsam', transform('libsam'), LIBS=['c', 'm', 'dl'], build_dir="lib")
#env.Program('samiam', transform('samiam'), LIBPATH='libsam', LIBS='sam')
