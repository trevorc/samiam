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
    '-D_REENTRANT',
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

destdir = ARGUMENTS.get('DESTDIR', '')

dirs = {
    'install': {
	'bin': destdir + '/usr/bin',
	'lib': destdir + '/usr/lib',
    },
    'build': {
	'bin': '#build/samiam',
	'lib': '#build/libsam',
    },
}

version_file = file('VERSION')
try:
    version = version_file.readline().strip()
finally:
    version_file.close()

if int(ARGUMENTS.get('debug', debug_default)):
    ccflags = ' '.join(debug + cflags)
else:
    ccflags = ' '.join(release + cflags)

SConscript('src/SConscript', ['ccflags', 'dirs', 'version'], build_dir='build', duplicate=0)
