/*
 * $Id$
 *
 * $Log$
 * Revision 1.1  2006/12/25 00:21:39  trevor
 * New SDK suite of headers.
 *
 */

#ifndef LIBSAM_EXECUTE_H
#define LIBSAM_EXECUTE_H

/**
 *  The various types allowed in sam memory locations.
 */
typedef enum {
    SAM_ML_TYPE_NONE,	/**< A null operand. */
    SAM_ML_TYPE_INT,	/**< An integer type. */
    SAM_ML_TYPE_FLOAT,	/**< An IEEE 754 floating point number. */
    SAM_ML_TYPE_SA,	/**< A memory address pointing to a location on
			 *   the stack. */
    SAM_ML_TYPE_HA,	/**< A memory address pointing to a location on
			 *   the heap. */
    SAM_ML_TYPE_PA	/**< A program address pointing to an
			 *   instruction in the source file. */
} sam_ml_type;

/** A value on the stack or heap. */
typedef union {
    sam_int   i;
    sam_float f;
    sam_pa    pa;
    sam_ha    ha;
    sam_sa    sa;
} sam_ml_value;

/** An element on the stack or on the heap. */
typedef struct {
    unsigned  type: 8;
    sam_ml_value value;
} sam_ml;

/** A pointer to an element on the heap or the stack. */
typedef struct {
    sam_bool stack; /**< Flag indicating whether the pointer is on the
		     *	 stack or heap. */
    union {
	sam_ha ha;
	sam_sa sa;
    } index;	    /**< The value of the pointer. */
} sam_ma;

typedef struct _sam_execution_state sam_execution_state;

extern sam_error sam_error_optype(/*@in@*/ sam_execution_state *s);
extern sam_error sam_error_segmentation_fault(sam_ma ma);
extern sam_error sam_error_free(sam_ha index);
extern sam_error sam_error_stack_underflow(void);
extern sam_error sam_error_stack_overflow(void);
extern sam_error sam_error_no_memory(void);
extern sam_error sam_error_type_conversion(sam_ml_type to, sam_ml_type found, sam_ml_type expected);
extern sam_error sam_error_unknown_identifier(const char *name);
extern sam_error sam_error_stack_input(/*@in@*/ sam_execution_state *s, const char *which, sam_ml_type found, sam_ml_type expected);
extern sam_error sam_error_negative_shift(/*@in@*/ sam_execution_state *s, sam_int i);
extern sam_error sam_error_division_by_zero(void);
extern sam_error sam_error_io(void);
extern void sam_error_uninitialized(/*@in@*/ sam_execution_state *s);
extern void sam_error_number_format(const char *buf);

sam_ml *sam_ml_new(sam_ml_value v, sam_ml_type t);
/*@null@*/ sam_ml *sam_pop(/*@in@*/ sam_execution_state *s);
sam_bool sam_push(/*@in@*/ sam_execution_state *s, /*@only@*/ /*@out@*/ sam_ml *m);
sam_ha sam_heap_alloc(/*@in@*/ sam_execution_state *s, size_t size);
sam_error sam_heap_dealloc(/*@in@*/ sam_execution_state *s, sam_ha index);
sam_bool sam_label_get(/*@in@*/ sam_execution_state *s, /*@out@*/ sam_pa *pa, const char *name);
/*@null@*/ sam_instruction *sam_instruction_get(/*@in@*/ sam_execution_state *s, sam_pa pa);

#endif /* LIBSAM_EXECUTE_H */
