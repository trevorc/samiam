#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

typedef struct {
    PyObject_HEAD
    sam_es *restrict es;
    char *file;
} ExecutionState;

static PyTypeObject ExecutionStateType;

/* Exceptions. */
static PyObject *SamError;
static PyObject *ParseError;

static void
ExecutionState_dealloc(ExecutionState *self)
{
    if (self->es) {
	sam_es_free(self->es);
    }
    free(self->file);
    self->ob_type->tp_free((PyObject *)self);
}

static int
ExecutionState_init(ExecutionState *self, PyObject *args, PyObject *kwds)
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
ExecutionState_bt_get(ExecutionState *restrict self)
{
    if (sam_es_bt_get(self->es)) {
	Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static int
ExecutionState_bt_set(ExecutionState *restrict self, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"Could not delete ExecutionState.bt.");
	return -1;
    }
    if (!PyBool_Check(v)) {
	PyErr_SetString(PyExc_TypeError,
			"ExecutionState.bt must be set to a Bool.");
	return -1;
    }
    sam_es_bt_set(self->es, v == Py_True);

    return 0;
}

static PyObject *
ExecutionState_fbr_get(ExecutionState *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_fbr_get(self->es));
}

static int
ExecutionState_fbr_set(ExecutionState *restrict self, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"Could not delete ExecutionState.fbr.");
	return -1;
    }
    if (!PyLong_Check(v)) {
	PyErr_SetString(PyExc_TypeError,
			"ExecutionState.fbr must be set to a long integer.");
	return -1;
    }
    sam_es_fbr_set(self->es, PyLong_AsLong(v));

    return 0;
}

static PyObject *
ExecutionState_pc_get(ExecutionState *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_pc_get(self->es));
}

static int
ExecutionState_pc_set(ExecutionState *restrict self, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"Could not delete ExecutionState.pc.");
	return -1;
    }
    if (!PyLong_Check(v)) {
	PyErr_SetString(PyExc_TypeError,
			"ExecutionState.pc must be set to a long integer.");
	return -1;
    }
    sam_es_pc_set(self->es, PyLong_AsLong(v));

    return 0;
}

static PyObject *
ExecutionState_sp_get(ExecutionState *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_sp_get(self->es));
}

static PyMethodDef ExecutionState_methods[] = {
    {0, 0, 0, 0}, /* Sentinel */
};

static PyObject *ExecutionState_iter(PyObject *es);

static PyGetSetDef ExecutionState_getset[] = {
    {"bt", (getter)ExecutionState_bt_get, (setter)ExecutionState_bt_set,
	"bt -- back trace bit.", NULL},
    {"fbr", (getter)ExecutionState_fbr_get, (setter)ExecutionState_fbr_set,
	"fbr -- frame base register.", NULL},
    {"pc", (getter)ExecutionState_pc_get, (setter)ExecutionState_pc_set,
	"pc -- program counter.", NULL},
    {"sp", (getter)ExecutionState_sp_get, NULL,
	"sp -- stack pointer.", NULL},
};

static PyTypeObject ExecutionStateType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.ExecutionState",
    .tp_basicsize = sizeof (ExecutionState),
    .tp_dealloc   = (destructor)ExecutionState_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
    .tp_iter	  = (getiterfunc)ExecutionState_iter,
    .tp_methods   = ExecutionState_methods,
    .tp_getset	  = ExecutionState_getset,
    .tp_init	  = (initproc)ExecutionState_init,
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
    if (PyType_Ready(&ExecutionStateType) < 0) {
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

    Py_INCREF(&ExecutionStateType);
    PyModule_AddObject(m, "ExecutionState",
		       (PyObject *)&ExecutionStateType);
}

typedef struct {
    PyObject_HEAD
    ExecutionState *es;
} ExecutionStateIterObject;

static void
ExecutionStateIter_dealloc(ExecutionStateIterObject *restrict self)
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
ExecutionStateIter_next(ExecutionStateIterObject *restrict self)
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

PyTypeObject ExecutionStateIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "executionstateiterator",
    .tp_basicsize = sizeof (ExecutionStateIterObject),
    .tp_dealloc	  = (destructor)ExecutionStateIter_dealloc,
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)ExecutionStateIter_next,
};

static PyObject *
ExecutionState_iter(PyObject *es)
{
    ExecutionStateIterObject *restrict self =
	PyObject_New(ExecutionStateIterObject, &ExecutionStateIterType);

    if (self == NULL) {
	return NULL;
    }

    Py_INCREF(es);
    self->es = (ExecutionState *)es;

    return (PyObject *)self;
}
