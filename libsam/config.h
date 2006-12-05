#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <float.h>

#define HAVE_MMAN_H	1
#define HAVE_UNISTD_H	1
#define HAVE_LOCALE_H	1
#undef HAVE_DLFCN_H

#undef SAM_EXTENSIONS

#if !defined(__WORDSIZE)
# define __WORDSIZE 32
#endif /* !__WORDSIZE */

/* define to float on 32 bit architectures, double on 64 bit. */
#if __WORDSIZE == 64
typedef double sam_float;
# define SAM_EPSILON DBL_EPSILON
#else
typedef float sam_float;
# define SAM_EPSILON FLT_EPSILON
#endif /* __WORDSIZE == 64 */

#if defined(__GNUC__)
# define UNUSED /*@unused@*/ __attribute__((__unused__))
#else 
# define UNUSED /*@unused@*/
#endif /* __GNUC__ */

#endif /* CONFIG_H */
