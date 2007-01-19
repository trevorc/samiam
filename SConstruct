# $Id$

debug_default = 1

cflags = [
    '-std=gnu99',
    '-pipe',
    '-W',
    '-Werror',
    '-Wall',
    '-Wmissing-prototypes',
    '-Wmissing-declarations',
#    '-Wstrict-prototypes',
    '-Wpointer-arith',
    '-Wnested-externs',
    '-Wdisabled-optimization',
    '-Wundef',
    '-Wendif-labels',
    '-Wshadow',
    '-Wcast-align',
    '-Wstrict-aliasing=2',
    '-Wwrite-strings',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wredundant-decls',
    '-Wformat',
#	'-msse3 -march=pentium-m',
#	'-mpower -mpowerpc -mcpu=powerpc', 
#	'-mthumb-interwork -msoft-float',
]
debug = [
    '-g',
    '-ggdb'
]
release = [
    '-O3',
]

if int(ARGUMENTS.get('debug', debug_default)):
    ccflags = ' '.join(debug + cflags)
else:
    ccflags = ' '.join(release + cflags)

dirs = {
    'install': {
	'bin': '/usr/local/bin',
	'lib': '/usr/local/lib',
    },
    'build': {
	'bin': '#build/samiam',
	'lib': '#build/libsam',
    },
}

SConscript('src/SConscript', ['ccflags', 'dirs'], build_dir='build', duplicate=0)
