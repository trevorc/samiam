#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

typedef struct {
    PyObject_HEAD
    sam_es *restrict es;
    char *file;
} ExecutionStateBase;

static PyTypeObject ExecutionStateBaseType;

/* Exceptions. */
static PyObject *SamError;
static PyObject *ParseError;

static void
ExecutionStateBase_dealloc(ExecutionStateBase *restrict self)
{
    if (self->es) {
	sam_es_free(self->es);
    }
    self->ob_type->tp_free((PyObject *)self);
}

static int
ExecutionState_init(ExecutionStateBase *restrict self,
		    PyObject *restrict args, PyObject *restrict kwds)
{
    static char *kwlist[] = {"file", NULL};
    char *file;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &file)) {
	return -1; 
    }

    self->es = sam_es_new(0, NULL);

    self->file = file;
    if (!sam_parse(self->es, file)) {
	PyErr_SetString(ParseError, "couldn't parse input file.");
	return -1;
    }

    return 0;
}

static PyObject *
ExecutionState_bt(ExecutionState *restrict self,
		  PyObject *restrict args)
{
    PyObject *arg = NULL;

    if (!PyArg_ParseTuple(args, "|O", &arg)) {
	return NULL;
    }
    if (arg == NULL) {
	if (sam_es_bt_get(self->es)) {
	    Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
    }
    if (!PyBool_Check(arg)) {
	return NULL;
    }
    sam_es_bt_set(self->es, arg == Py_True);

    Py_RETURN_NONE;
}

static PyObject *
ExecutionState_fbr(ExecutionState *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_fbr_get(self->es));
}

static PyObject *
ExecutionState_pc(ExecutionState *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_pc_get(self->es));
}

static PyObject *
ExecutionState_sp(ExecutionState *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_sp_get(self->es));
}

static PyMethodDef ExecutionState_methods[] = {
    {"bt", (PyCFunction)ExecutionState_bt, METH_VARARGS,
	"Get or set the back trace bit."},
    {"fbr", (PyCFunction)ExecutionState_fbr, METH_NOARGS,
	"Return the frame base register."},
    {"pc", (PyCFunction)ExecutionState_pc, METH_NOARGS,
	"Return the program counter."},
    {"sp", (PyCFunction)ExecutionState_sp, METH_NOARGS,
	"Return the stack pointer."},
    {0, 0, 0, 0}, /* Sentinel */
};

static PyObject *ExecutionState_iter(PyObject *es);

static PyTypeObject ExecutionStateType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.ExecutionState",
    .tp_basicsize = sizeof (ExecutionState),
    .tp_dealloc   = (destructor)ExecutionState_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
    .tp_iter	  = (getiterfunc)ExecutionState_iter,
    .tp_methods   = ExecutionState_methods,
    .tp_init	  = (initproc)ExecutionState_init,
    .tp_new	  = PyType_GenericNew,
};

#ifndef PyMODINIT_FUNC
# define PyMODINIT_FUNC void
#endif

static PyMethodDef module_methods[] = {
    {NULL}
};

PyMODINIT_FUNC
initsam(void)
{
    PyObject *m;

    if (PyType_Ready(&ExecutionStateType) < 0) {
	return;
    }

    m = Py_InitModule3("sam", module_methods, "Libsam Python bindings.");

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
