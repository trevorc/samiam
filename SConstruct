# $Id$

import glob
import os
import SCons.Node.FS

debug_default = 1

cflags = [
    '-std=gnu99',
    '-fgnu89-inline',
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
rpm_opt_flags = ARGUMENTS.get('RPM_OPT_FLAGS', '')

cflags += rpm_opt_flags.split()

prefix = '/usr'

dirs = {
    'install': {
	'bin': os.sep.join([destdir, prefix, 'bin']),
	'lib': os.sep.join([destdir, prefix, 'lib']),
	'locale': os.sep.join([destdir, prefix, 'share', 'locale']),
    },
    'build': {
	'bin': os.sep.join(['#build', 'samiam']),
	'lib': os.sep.join(['#build', 'libsam']),
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

def po_helper(po, pot):
    args = [
	'msgmerge',
	'--update',
	po,
	pot,
    ]
    print 'Updating ' + po
    return os.spawnvp(os.P_WAIT, 'msgmerge', args)

def mo_builder(target, source, env):
    po_helper(source[0].get_path(), source[1].get_path())
    args = [
	'msgfmt',
	'-c',
	'-o',
	target[0].get_path(),
	source[0].get_path()
    ]
    return os.spawnvp (os.P_WAIT, 'msgfmt', args)

def pot_builder(target, source, env):
    args = [
	'xgettext', 
	 '--keyword=_',
	 '--keyword=N_',
	 '--from-code=UTF-8',
	 '-o', target[0].get_path(), 
	 "--default-domain=" + env['PACKAGE'],
	 '--copyright-holder="Trevor Caira"'
    ]
    args += [src.get_path() for src in source]
    print args

    return os.spawnvp(os.P_WAIT, 'xgettext', args)

def i18n(env, sources):
    domain = env['PACKAGE']
    potfile = env['POTFILE']

    env.PotBuild(potfile, sources)

    p_oze = [os.path.basename(po) for po in glob.glob('po/*.po')]
    languages = [po.replace('.po', '') for po in p_oze]
    m_oze = [po.replace('.po', '.mo') for po in p_oze]

    for mo in m_oze:
	po = 'po/' + mo.replace ('.mo', '.po')
	mo_b = env.MoBuild(mo, [po, potfile])
	env.Alias('install', env.Install(dirs['install']['locale'], mo_b))

#    for po_file in p_oze:
#	env.PotBuild(po_file, ['po/' + po_file, potfile])
#	mo_file = po_file.replace(".po", ".mo")
#	env.Alias('install', env.MoBuild(mo_file, po_file))

    for lang in languages:
	modir = os.path.join(dirs['install']['locale'], lang, 'LC_MESSAGES')
	moname = domain + '.mo'
	env.Alias('install', env.InstallAs(os.path.join(modir, moname), lang + '.mo'))

conf = Configure(
    Environment(CCFLAGS=ccflags,
		CPPPATH='../include',
		LIBPATH=dirs['build']['lib']
))

mo_bld = Builder(action=mo_builder)
conf.env.Append(BUILDERS = {'MoBuild': mo_bld})

pot_bld = Builder(action=pot_builder)
conf.env.Append(BUILDERS = {'PotBuild': pot_bld})

conf.env.Clean('scrub', ['.sconf_temp', 'config.log', 'build'])

SConscript('src/SConscript', ['conf', 'ccflags', 'dirs', 'version', 'i18n'], build_dir='build', duplicate=0)
