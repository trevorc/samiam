#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

static PyTypeObject ExecutionStateType;
static PyObject *ExecutionState_iter(PyObject *es);
static PyObject *SamError;
static PyObject *ParseError;

typedef struct {
    PyObject_HEAD
    char *inst;
    PyObject *labels; /* tuple */
} Instruction;

/* Actually, all of these objects should probably have identical
 * PyTypeObject structs except for the methods. */
typedef struct {
    PyObject_HEAD
    sam_es *es;
} EsRefObj;

/* TODO Do sequences automatically get iterators, or do you have to
 * define one yourself even though there is a sane default? */
/* Sequence of the SaM program code */
typedef EsRefObj Instructions;

static PyTypeObject InstructionsType;

static PyTypeObject InstructionsType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Instructions",
    .tp_basicsize = sizeof (Instructions),
    .tp_dealloc   = (destructor)Instructions_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam program code",
    .tp_iter	  = (getiterfunc)Instructions_iter,
    .tp_as_squence = Instructions_sequence_methods,
    .tp_methods   = Instructions_methods,
    .tp_getset	  = Instructions_getset,
    .tp_init	  = (initproc)Instructions_init,
    .tp_new	  = PyType_GenericNew,
};

/* Sequence of the SaM stack */
typedef EsRefObj Stack;

static PyTypeObject StackType;

static PyTypeObject StackType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Stack",
    .tp_basicsize = sizeof (Stack),
    .tp_dealloc   = (destructor)Stack_dealloc,
    .tp_flags	  = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam stack",
    .tp_iter	  = (getiterfunc)Stack_iter,
    .tp_as_sequence = Stack_sequence_methods,
    .tp_methods	  = Stack_methods,
    .tp_getset	  = Stack_getset,
    .tp_init	  = (initproc)Stack_init,
    .tp_new	  = PyType_GenericNew,
};

/* Sequence of the SaM heap */
typedef EsRefObj Heap;

static PyTypeObject HeapType;

static PyTypeObject HeapType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Heap",
    .tp_basicsize = sizeof (Heap),
    .tp_dealloc   = (destructor)Heap_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
    .tp_iter	  = (getiterfunc)Heap_iter,
    .tp_as_sequence = Heap_sequence_methods,
    .tp_methods   = Heap_methods,
    .tp_getset	  = Heap_getset,
    .tp_init	  = (initproc)Heap_init,
    .tp_new	  = PyType_GenericNew,
};

typedef struct {
    PyObject_HEAD
    sam_es *es;
    char *file;
} Program;

static void
Program_dealloc(Program *self)
{
    if (self->es) {
	sam_es_free(self->es);
    }
    free(self->file);
    self->ob_type->tp_free((PyObject *)self);
}

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

static PyObject *
Program_bt_get(Program *restrict self)
{
    if (sam_es_bt_get(self->es)) {
	Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

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

static PyObject *
Program_fbr_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_fbr_get(self->es));
}

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

static PyObject *
Program_pc_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_pc_get(self->es));
}

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

static PyObject *
Program_sp_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_sp_get(self->es));
}

static PyMethodDef Program_methods[] = {
    {0, 0, 0, 0}, /* Sentinel */
};

static PyObject *Program_iter(PyObject *es);

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

static PyMethodDef module_methods[] = {
    {0, 0, 0, 0}
};

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

typedef struct {
    PyObject_HEAD
    Program *es;
} ProgramIterObject;

static void
ProgramIter_dealloc(ProgramIterObject *restrict self)
{
    Py_DECREF(self->es);

#ifndef NDEBUG
    printf("self:\t%p\n", (void *)self);
    printf("ob_type:\t%p\n", (void *)self->ob_type);
    printf("tp_free:\t%p\n", (void *)self->ob_type->tp_free);
#endif

    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *
ProgramIter_next(ProgramIterObject *restrict self)
{
#ifndef NDEBUG
    printf("self:\t\t%p\n"
	   "self->es:\t%p\n"
	   "self->es->es:\t%p\n",
	   (void *)self,
	   (void *)self->es,
	   (void *)self->es->es);
#endif

    if (sam_es_pc_get(self->es->es) >=
	sam_es_instructions_len(self->es->es)) {
	return NULL;
    }

#ifndef NDEBUG
    sam_instruction *restrict cur = sam_es_instructions_cur(self->es->es);
    printf("instruction:\t%p\n"
	   "handler:\t%p\n",
	   (void *)cur,
	   (void *)cur->handler);
#endif

    long err = sam_es_instructions_cur(self->es->es)->handler(self->es->es);

    PyObject *restrict rv = PyLong_FromLong(err);
    sam_es_pc_pp(self->es->es);

    return rv;
}

PyTypeObject ProgramIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "programiterator",
    .tp_basicsize = sizeof (ProgramIterObject),
    .tp_dealloc	  = (destructor)ProgramIter_dealloc,
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)ProgramIter_next,
};

static PyObject *
Program_iter(PyObject *es)
{
    ProgramIterObject *restrict self =
	PyObject_New(ProgramIterObject, &ProgramIterType);

    if (self == NULL) {
	return NULL;
    }

    Py_INCREF(es);
    self->es = (Program *)es;

    return (PyObject *)self;
}
