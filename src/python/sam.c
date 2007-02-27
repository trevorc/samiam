#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

/* Exceptions {{{1 */
static PyTypeObject ExecutionStateType;
static PyObject *SamError;
static PyObject *ParseError;

/* EsRefObj {{{1 */
/* typedef EsRefObj {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
} EsRefObj;

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
/* Deallocator for types which keep track of a Program. */
static void
ProgRefType_dealloc(ProgRefType *restrict self)
{
    Py_DECREF(self->prog);
    self->ob_type->tp_free((PyObject *)self);
}

/* Instructions {{{1 */
/* typedef Instruction {{{2 */
typedef struct {
    PyObject_HEAD
    char *inst;
    PyObject *labels; /* tuple */
} Instruction;

/* typedef Instructions {{{2 */
/* Sequence of the SaM program code */
typedef EsRefObj Instructions;

/* PyTypeObject InstructionsType {{{2 */
static PyTypeObject InstructionsType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Instructions",
    .tp_basicsize = sizeof (Instructions),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam program code",
    .tp_iter	  = (getiterfunc)Instructions_iter,
    .tp_as_sequence = Instructions_sequence_methods,
    .tp_methods   = Instructions_methods,
    .tp_getset	  = Instructions_getset,
    .tp_init	  = (initproc)Instructions_init,
    .tp_new	  = PyType_GenericNew,
};

/* Stack {{{1 */
/* typedef Stack {{{2 */
typedef EsRefObj Stack;

/* PyTypeObject StackType {{{2 */
static PyTypeObject StackType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Stack",
    .tp_basicsize = sizeof (Stack),
    .tp_flags	  = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam stack",
    .tp_iter	  = (getiterfunc)Stack_iter,
    .tp_as_sequence = Stack_sequence_methods,
    .tp_methods	  = Stack_methods,
    .tp_getset	  = Stack_getset,
    .tp_init	  = (initproc)Stack_init,
    .tp_new	  = PyType_GenericNew,
};

/* Heap {{{1 */
/* typedef Heap {{{2 */
/* Sequence of the SaM heap */
typedef EsRefObj Heap;

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
    .tp_getset	  = Heap_getset,
    .tp_init	  = (initproc)Heap_init,
    .tp_new	  = PyType_GenericNew,
};

/* ProgramIter {{{1 */
/* typedef ProgramIterObject {{{2 */
typedef ProgRefType ProgramIterObject;

/* ProgramIter_next () {{{2 */
static PyObject *
ProgramIter_next(ProgramIterObject *restrict self)
{
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
	"bt -- back trace bit.", NULL},
    {"fbr", (getter)Program_fbr_get, (setter)Program_fbr_set,
	"fbr -- frame base register.", NULL},
    {"pc", (getter)Program_pc_get, (setter)Program_pc_set,
	"pc -- program counter.", NULL},
    {"sp", (getter)Program_sp_get, NULL,
	"sp -- stack pointer.", NULL},
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
