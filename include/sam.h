/** Global program behavior defaults configurable by command line
 * options. */
typedef enum {
    quiet = 1 << 0	/**< Suppress verbose error messages. Library
			 *   errors are however not suppressed. */
} sam_options;

/** Exit codes for main() in case of error. */
typedef enum {
    SAM_EMPTY_STACK = -1,   /**< The stack was empty after program
typedef int (*sam_read_int_func)(int *n);
typedef int (*sam_read_float_func)(double *f);
typedef int (*sam_read_char_func)(char *c);
typedef int (*sam_read_str_func)(char **s);
typedef int (*sam_write_str_func)(char *s);

/**
 *  Input/output callbacks. Used by various i/o opcodes.
 */
    /*@null@*/ /*@dependent@*/ sam_read_char_func   read_char_func;
    sam_read_int_func	    read_int_func;
    sam_read_float_func	    read_float_func;
    sam_read_char_func	    read_char_func;
    sam_read_str_func	    read_str_func;
    sam_write_int_func	    write_int_func;
    sam_write_float_func    write_float_func;
    sam_write_char_func	    write_char_func;
    sam_write_str_func	    write_str_func;
 *  Entry point to libsam.
 *

		    /*@null@*/ const char  *file,
