#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

/* Exceptions {{{1 */
/* PyTypeObject ExecutionStateType {{{2 */
static PyTypeObject ExecutionStateTypez;

/* PyObject SamError {{{2 */
static PyObject *SamError;

/* PyObject ParseError {{{2 */
static PyObject *ParseError;

/* EsRefObj {{{1 */
/* typedef EsRefObj {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
} EsRefObj;

/* typedef EsRefIterObj {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    size_t idx;
} EsRefIterObj;

/* typedef Program {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    char *file;
} Program;

/* ProgRefType {{{1 */
/* typedef ProgRefType {{{2 */
typedef struct {
    PyObject_HEAD
    Program *prog;
} ProgRefType;

/* ProgRefType_dealloc () {{{2 */
/* Deallocator for non-iterator types which keep track of a Program. */
static void
ProgRefType_dealloc(ProgRefType *restrict self)
{
    Py_DECREF(self->prog);
    self->ob_type->tp_free((PyObject *)self);
}

/* ProgRefIterType {{{1 */
/* typedef ProgRefIterType {{{2 */
typedef struct {
    PyObject_HEAD
    Program *prog;
    size_t idx;
} ProgRefIterType;

/* ProgRefIterType_dealloc () {{{2 */
/* Deallocator for iterator types which keep track of a Program. */
static void
ProgRefIterType_dealloc(ProgRefIterType *restrict self)
{
    Py_DECREF(self->prog);
    self->ob_type->tp_free((PyObject *)self);
}

/* Instruction {{{1 */
/* typedef Instruction {{{2 */
typedef struct {
    PyObject_HEAD
    const char *inst;
    PyObject *labels; /* tuple */
} Instruction;

/* Instruction_code_get () {{{2 */
static PyObject *
Instruction_assembly_get(Instruction *restrict self)
{
    return PyString_FromString(self->inst);
}

/* Instruction_labels_get () {{{2 */
static PyObject *
Instruction_labels_get(Instruction *restrict self)
{
    Py_INCREF(self->labels);

    return self->labels;
}

/* PyGetSetDef Instruction_getset {{{2 */
static PyGetSetDef Instruction_getset[] = {
    /* TODO call this assembly? Something better? */
    /* TODO setters... later? */
    {"assembly", (getter)Instruction_assembly_get, NULL,
	"the line of SaM code for this.", NULL},
    {"labels", (getter)Instruction_labels_get, NULL,
	"the labels for this.", NULL}
};

/* PyTypeObject InstructionType {{{2 */
static PyTypeObject InstructionType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Instruction",
    .tp_basicsize = sizeof (Instruction),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam instruction (one line of a program)",
    .tp_getset	  = Instruction_getset,
    .tp_new	  = PyType_GenericNew,
};

/* InstructionsIter {{{1 */
/* typedef InstructionsIterObject {{{2 */
typedef ProgRefIterType InstructionsIterObject;

/* InstructionsIter_next () {{{2 */
static PyObject *
InstructionsIter_next(InstructionsIterObject *restrict self)
{
    if (self->idx >= sam_es_instructions_len(self->prog->es)) {
	return NULL;
    }

    // TODO get and make Instruction object
}

/* PyTypeObject InstructionsIterType {{{2 */
PyTypeObject InstructionsIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "instructionsiterator",
    .tp_basicsize = sizeof (InstructionsIterObject),
    .tp_dealloc	  = (destructor)ProgRefIterType_dealloc,
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)InstructionsIter_next,
};

/* Instructions_iter () {{{2 */
static PyObject *
Instructions_iter(PyObject *prog)
{
    InstructionsIterObject *restrict self =
	PyObject_New(InstructionsIterObject, &InstructionsIterType);

    if (self == NULL) {
	return NULL;
    }

    Py_INCREF(prog);
    self->prog = (Program *)prog;

    return (PyObject *)self;
}
/* Instructions {{{1 */
/* typedef Instructions {{{2 */
/* Sequence of the SaM program code */
typedef EsRefObj Instructions;

/* Instructions_length {{{2 */
static long
Instructions_length(Instructions *self)
{
    return sam_es_instructions_len(self->es);
}

static Instruction *
Instructions_item(Instructions *self, unsigned i)
{
    /* i is unsigned, no check for i < 0 */
    if(i >= sam_es_instructions_len(self->es))
    {
	PyErr_SetNone(PyExc_IndexError);
	return NULL;
    }

    // TODO can we just cast to sam_pa?
    sam_instruction *inst = sam_es_instructions_get(self->es, (sam_pa) i);

    Instruction *rv = PyObject_New(Instruction, &InstructionType);
    rv->inst = inst->name;
    // TODO get labels
    rv->labels = PyTuple_New(0);
    return rv;
}

/* PySquenceMethods Instructions_sequence_methods {{{2 */
static PySequenceMethods Instructions_sequence_methods = {
    .sq_length = (inquiry)Instructions_length,
//    /*(binaryfunc)*/0,			/*sq_concat*/
//    /*(intargfunc)*/0,		/*sq_repeat*/
    .sq_item = (intargfunc)Instructions_item,
//    /*(intintargfunc)*/0,		/*sq_slice*/
//    /*(intobjargproc)*/0,		/*sq_ass_item*/
//    /*(intintobjargproc)*/0,	/*sq_ass_slice*/
};

/* PyTypeObject InstructionsType {{{2 */
static PyTypeObject InstructionsType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Instructions",
    .tp_basicsize = sizeof (Instructions),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam program code",
    .tp_iter	  = (getiterfunc)Instructions_iter,
    .tp_as_sequence = &Instructions_sequence_methods,
    .tp_new	  = PyType_GenericNew,
};

/* Type {{{1 */
/* typedef Type {{{2 */
typedef struct {
    PyObject_HEAD
    sam_ml_type type;
} Type;

/* Type_str {{{2 */
static PyObject *
Type_str(Type *restrict self)
{
    switch (self->type) {
	case SAM_ML_TYPE_NONE:
	    return PyString_FromString("None");
	case SAM_ML_TYPE_INT:
	    return PyString_FromString("Int");
	case SAM_ML_TYPE_FLOAT:
	    return PyString_FromString("Float");
	case SAM_ML_TYPE_SA:
	    return PyString_FromString("SA");
	case SAM_ML_TYPE_HA:
	    return PyString_FromString("HA");
	case SAM_ML_TYPE_PA:
	    return PyString_FromString("PA");
	default:
	    // TODO correct error procedure?
	    return NULL;
    }
}

/* Type_asChar {{{2 */
static PyObject *
Type_asChar(Type *restrict self)
{
    switch (self->type) {
	case SAM_ML_TYPE_NONE:
	    return PyString_FromString("N");
	case SAM_ML_TYPE_INT:
	    return PyString_FromString("I");
	case SAM_ML_TYPE_FLOAT:
	    return PyString_FromString("F");
	case SAM_ML_TYPE_SA:
	    return PyString_FromString("S");
	case SAM_ML_TYPE_HA:
	    return PyString_FromString("H");
	case SAM_ML_TYPE_PA:
	    return PyString_FromString("P");
	default:
	    // TODO correct error procedure?
	    return NULL;
    }
}

/* PyMethodDef Type_methods {{{2 */
static PyMethodDef Type_methods[] = {
    {"asChar", (PyCFunction)Type_asChar, METH_NOARGS,
	"Returns a single character string representing the type"},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};
// TODO Does type need more helper methods? We will see when using it.

/* PyTypeObject TypeType {{{2 */
static PyTypeObject TypeType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Type",
    .tp_basicsize = sizeof (Type),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam memory value type",
    .tp_str	  = (reprfunc)Type_str,
    .tp_methods   = Type_methods,
    .tp_new	  = PyType_GenericNew,
};

/* Type_create {{{2 */
static Type *
Type_create(sam_ml_type type)
{
    Type *rv = PyObject_New(Type, &TypeType);
    rv->type = type;

    return rv;
}

/* Value {{{1 */
/* typedef Value {{{2 */
typedef struct {
    PyObject_HEAD
    sam_ml value;
} Value;

/* Value_value_get () {{{2 */
static PyObject *
Value_value_get(Value *restrict self)
{
    switch (self->value.type) {
	case SAM_ML_TYPE_INT:
	    return PyLong_FromLong(self->value.value.i);
	case SAM_ML_TYPE_FLOAT:
	    return PyFloat_FromDouble(self->value.value.f);
	case SAM_ML_TYPE_SA:
	    return PyLong_FromLong(self->value.value.pa);
	case SAM_ML_TYPE_HA:
	    return PyLong_FromLong(self->value.value.ha);
	case SAM_ML_TYPE_PA:
	    return PyLong_FromLong(self->value.value.sa);
	default:
	    // TODO is this an exception?
	    return NULL;
    }
}

/* Value_type_get () {{{2 */
static PyObject *
Value_type_get(Value *restrict self)
{
    // TODO reference counting?
    switch (self->value.type) {
	case SAM_ML_TYPE_NONE:
	    return PyObject_GetAttrString(&TypeType, "none");
	case SAM_ML_TYPE_INT:
	    return PyObject_GetAttrString(&TypeType, "int");
	case SAM_ML_TYPE_FLOAT:
	    return PyObject_GetAttrString(&TypeType, "float");
	case SAM_ML_TYPE_SA:
	    return PyObject_GetAttrString(&TypeType, "sa");
	case SAM_ML_TYPE_HA:
	    return PyObject_GetAttrString(&TypeType, "ha");
	case SAM_ML_TYPE_PA:
	    return PyObject_GetAttrString(&TypeType, "pa");
	default:
	    // TODO correct error procedure?
	    return NULL;
    }
}

/* PyGetSetDef Value_getset {{{2 */
static PyGetSetDef Value_getset[] = {
    {"value", (getter)Value_value_get, NULL,
	"value -- numerical value (PyLong or PyFloat).", NULL},
    {"type", (getter)Value_type_get, NULL,
	"type -- type of value.", NULL}
};

/* PyTypeObject ValueType {{{2 */
static PyTypeObject ValueType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Value",
    .tp_basicsize = sizeof (Value),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam memory value (one Sam word)",
    .tp_getset	  = Value_getset,
    .tp_new	  = PyType_GenericNew,
};

/* Value_create () {{{2 */
static Value *
Value_create(sam_ml val)
{
    Value *rv = PyObject_New(Value, &ValueType);
    rv->value = val;

    return rv;
}

/* StackIter {{{1*/
/* typedef StackIter {{{2 */
typedef ProgRefIterType StackIter;

/* StackIter_next () {{{2 */
static Value *
StackIter_next(StackIter *restrict self)
{
    if (self->idx >= sam_es_stack_len(self->prog->es)) {
	return NULL;
    }

    return Value_create(*sam_es_stack_get(self->prog->es, self->idx));
}

/* PyTypeObject StackIterType {{{2 */
PyTypeObject StackIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "heapiterator",
    .tp_basicsize = sizeof (StackIter),
    .tp_dealloc	  = (destructor)ProgRefIterType_dealloc,
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)StackIter_next,
};

/* Stack {{{1 */
/* typedef Stack {{{2 */
/* Sequence of the SaM heap */
typedef EsRefObj Stack;

/* Stack_length {{{2 */
static long
Stack_length(Stack *restrict self)
{
    return sam_es_stack_len(self->es);
}

/* Stack_item {{{2 */
static Value *
Stack_item(Stack *restrict self, unsigned i)
{
    if (i >= sam_es_stack_len(self->es)) {
	return NULL;
    }

    return Value_create(*sam_es_stack_get(self->es, i));
}

/* PySquenceMethods Stack_sequence_methods {{{2 */
static PySequenceMethods Stack_sequence_methods = {
    .sq_length = (inquiry)Stack_length,
    /*(binaryfunc)*/0,			/*sq_concat*/
    /*(intargfunc)*/0,		/*sq_repeat*/
    .sq_item = (intargfunc)Stack_item,
    /*(intintargfunc)*/0,		/*sq_slice*/
    /*(intobjargproc)*/0,		/*sq_ass_item*/
    /*(intintobjargproc)*/0,	/*sq_ass_slice*/
};

/* PyTypeObject StackType {{{2 */
static PyTypeObject StackType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Stack",
    .tp_basicsize = sizeof (Stack),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
    .tp_iter	  = (getiterfunc)Stack_iter,
    .tp_as_sequence = Stack_sequence_methods,
    .tp_new	  = PyType_GenericNew,
};

/* HeapIter {{{1*/
/* typedef HeapIter {{{2 */
typedef EsRefIterType HeapIter;

/* HeapIter_next () {{{2 */
static Value *
HeapIter_next(HeapIter *restrict self)
{
    if (self->idx >= sam_es_heap_len(self->es)) {
	return NULL;
    }

    return Value_create(*sam_es_heap_get(self->es, self->idx));
}

/* PyTypeObject HeapIterType {{{2 */
PyTypeObject HeapIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "heapiterator",
    .tp_basicsize = sizeof (HeapIter),
    .tp_dealloc	  = (destructor)ProgRefIterType_dealloc,
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)HeapIter_next,
};

/* Heap {{{1 */
/* typedef Heap {{{2 */
/* Sequence of the SaM heap */
typedef EsRefObj Heap;

/* Heap_length {{{2 */
static long
Heap_length(Heap *self)
{
    return sam_es_heap_len(self->es);
}

/* Heap_item {{{2 */
static Value *
Heap_item(Heap *self, unsigned i)
{
    if (i >= sam_es_heap_len(self->es)) {
	return NULL;
    }

    return Value_create(*sam_es_heap_get(self->es, i));
}

/* PySquenceMethods Heap_sequence_methods {{{2 */
static PySequenceMethods Heap_sequence_methods = {
    .sq_length = (inquiry)Heap_length,
    /*(binaryfunc)*/0,			/*sq_concat*/
    /*(intargfunc)*/0,		/*sq_repeat*/
    .sq_item = (intargfunc)Heap_item,
    /*(intintargfunc)*/0,		/*sq_slice*/
    /*(intobjargproc)*/0,		/*sq_ass_item*/
    /*(intintobjargproc)*/0,	/*sq_ass_slice*/
};

/* PyTypeObject HeapType {{{2 */
static PyTypeObject HeapType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Heap",
    .tp_basicsize = sizeof (Heap),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
    .tp_iter	  = (getiterfunc)Heap_iter,
    .tp_as_sequence = Heap_sequence_methods,
    .tp_methods   = Heap_methods,
    .tp_new	  = PyType_GenericNew,
};

/* ProgramIter {{{1 */
/* typedef ProgramIterObject {{{2 */
typedef ProgRefType ProgramIterObject;

/* ProgramIter_next () {{{2 */
static PyObject *
ProgramIter_next(ProgramIterObject *restrict self)
{
    /* TODO: Does this work? If it should, yell at Trevor.*/
    if (sam_es_pc_get(self->prog->es) >=
	sam_es_instructions_len(self->prog->es)) {
	return NULL;
    }

    long err = sam_es_instructions_cur(self->prog->es)->handler(self->prog->es);

    PyObject *restrict rv = PyLong_FromLong(err);
    sam_es_pc_pp(self->prog->es);

    return rv;
}


/* PyTypeObject ProgramIterType {{{2 */
PyTypeObject ProgramIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "programiterator",
    .tp_basicsize = sizeof (ProgramIterObject),
    .tp_dealloc	  = (destructor)ProgRefType_dealloc,
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)ProgramIter_next,
};

/* Program_iter () {{{2 */
static PyObject *
Program_iter(PyObject *prog)
{
    ProgramIterObject *restrict self =
	PyObject_New(ProgramIterObject, &ProgramIterType);

    if (self == NULL) {
	return NULL;
    }

    Py_INCREF(prog);
    self->prog = (Program *)prog;

    return (PyObject *)self;
}
/* Program {{{1 */
/* Program_bt_get () {{{2 */
static PyObject *
Program_bt_get(Program *restrict self)
{
    if (sam_es_bt_get(self->es)) {
	Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

/* Program_bt_set () {{{2 */
static int
Program_bt_set(Program *restrict self, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"Could not delete Program.bt.");
	return -1;
    }
    if (!PyBool_Check(v)) {
	PyErr_SetString(PyExc_TypeError,
			"Program.bt must be set to a Bool.");
	return -1;
    }
    sam_es_bt_set(self->es, v == Py_True);

    return 0;
}

/* Program_fbr_get () {{{2 */
static PyObject *
Program_fbr_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_fbr_get(self->es));
}

/* Program_fbr_get () {{{2 */
static int
Program_fbr_set(Program *restrict self, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"Could not delete Program.fbr.");
	return -1;
    }
    if (!PyLong_Check(v)) {
	PyErr_SetString(PyExc_TypeError,
			"Program.fbr must be set to a long integer.");
	return -1;
    }
    sam_es_fbr_set(self->es, PyLong_AsLong(v));

    return 0;
}

/* Program_pc_get () {{{2 */
static PyObject *
Program_pc_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_pc_get(self->es));
}

/* Program_pc_set () {{{2 */
static int
Program_pc_set(Program *restrict self, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"Could not delete Program.pc.");
	return -1;
    }
    if (!PyLong_Check(v)) {
	PyErr_SetString(PyExc_TypeError,
			"Program.pc must be set to a long integer.");
	return -1;
    }
    sam_es_pc_set(self->es, PyLong_AsLong(v));

    return 0;
}

/* Program_sp_get () {{{2 */
static PyObject *
Program_sp_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_sp_get(self->es));
}

/* PyGetSetDef Program_getset {{{2 */
static PyGetSetDef Program_getset[] = {
    {"bt", (getter)Program_bt_get, (setter)Program_bt_set,
	"back trace bit.", NULL},
    {"fbr", (getter)Program_fbr_get, (setter)Program_fbr_set,
	"frame base register.", NULL},
    {"pc", (getter)Program_pc_get, (setter)Program_pc_set,
	"program counter.", NULL},
    {"sp", (getter)Program_sp_get, NULL,
	"stack pointer.", NULL},
};

/* Program_dealloc () {{{2 */
static void
Program_dealloc(Program *self)
{
    if (self->es) {
	sam_es_free(self->es);
    }
    free(self->file);
    self->ob_type->tp_free((PyObject *)self);
}

/* PyMethodDef Program_methods {{{2 */
static PyMethodDef Program_methods[] = {
    {0, 0, 0, 0}, /* Sentinel */
};

/* Program_init () {{{2 */
static int
Program_init(Program *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"file", NULL};
    char *file;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &file)) {
	return -1; 
    }

    self->es = sam_es_new(1, NULL);

    self->file = strdup(file);
    if (!sam_parse(self->es, file)) {
	PyErr_SetString(ParseError, "couldn't parse input file.");
	return -1;
    }

    return 0;
}

/* PyTypeObject ProgramType {{{2 */
static PyTypeObject ProgramType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Program",
    .tp_basicsize = sizeof (Program),
    .tp_dealloc   = (destructor)Program_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
    .tp_iter	  = (getiterfunc)Program_iter,
    .tp_methods   = Program_methods,
    .tp_getset	  = Program_getset,
    .tp_init	  = (initproc)Program_init,
    .tp_new	  = PyType_GenericNew,
};

#ifndef PyMODINIT_FUNC
# define PyMODINIT_FUNC void
#endif


/* module {{{1 */
/* PyMethodDef module_methods {{{2 */
static PyMethodDef module_methods[] = {
    {0, 0, 0, 0}
};

/* initsam () {{{2 */
PyMODINIT_FUNC
initsam(void)
{
    if (PyType_Ready(&TypeType) < 0) {
	return;
    }
    // TODO right way to do types?
    PyObject_SetAttrString(&TypeType, "none", Type_create(SAM_ML_TYPE_NONE));
    PyObject_SetAttrString(&TypeType, "int", Type_create(SAM_ML_TYPE_INT));
    PyObject_SetAttrString(&TypeType, "float", Type_create(SAM_ML_TYPE_FLOAT));
    PyObject_SetAttrString(&TypeType, "sa", Type_create(SAM_ML_TYPE_SA));
    PyObject_SetAttrString(&TypeType, "ha", Type_create(SAM_ML_TYPE_HA));
    PyObject_SetAttrString(&TypeType, "pa", Type_create(SAM_ML_TYPE_PA));

    if (PyType_Ready(&ValueType) < 0) {
	return;
    }
    if (PyType_Ready(&InstructionType) < 0) {
	return;
    }
    if (PyType_Ready(&InstructionsIterType) < 0) {
	return;
    }
    if (PyType_Ready(&InstructionsType) < 0) {
	return;
    }
    if (PyType_Ready(&StackIterType) < 0) {
	return;
    }
    if (PyType_Ready(&StackType) < 0) {
	return;
    }
    if (PyType_Ready(&HeapIterType) < 0) {
	return;
    }
    if (PyType_Ready(&HeapType) < 0) {
	return;
    }
    if (PyType_Ready(&ProgramIterType) < 0) {
	return;
    }
    if (PyType_Ready(&ProgramType) < 0) {
	return;
    }

    PyObject *restrict m =
	Py_InitModule3("sam", module_methods, "Libsam Python bindings.");

    SamError = PyErr_NewException("sam.SamError", NULL, NULL);
    if (SamError != NULL) {
	Py_INCREF(SamError);
	PyModule_AddObject(m, "SamError", SamError);
    }

    ParseError = PyErr_NewException("sam.ParseError", SamError, NULL);
    if (ParseError != NULL) {
	Py_INCREF(ParseError);
	PyModule_AddObject(m, "ParseError", ParseError);
    }

    Py_INCREF(&ProgramType);
    PyModule_AddObject(m, "Program",
		       (PyObject *)&ProgramType);
}
