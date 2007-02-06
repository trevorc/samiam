from distutils.core import setup, Extension

include_dirs=['../include']
library_dirs=['../../build/libsam']
libraries=['sam']

setup(name="sam",
      version="1.0",
      author="Trevor Caira",
      description="Libsam Python bindings",
      ext_modules=[Extension("sam", ["sam.c"],
			     include_dirs=include_dirs,
			     library_dirs=library_dirs,
			     libraries=libraries)])
