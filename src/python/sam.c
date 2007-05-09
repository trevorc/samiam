#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

/* Exceptions {{{1 */
/* PyTypeObject ExecutionStateType {{{2 */
static PyTypeObject ExecutionStateType;

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

/* typedef EsRefIterType {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    size_t idx;
} EsRefIterType;

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
typedef EsRefIterType InstructionsIterObject;

/* InstructionsIter_next () {{{2 */
static PyObject *
InstructionsIter_next(InstructionsIterObject *restrict self)
{
    if (self->idx >= sam_es_instructions_len(self->es)) {
	return NULL;
    }

    Instruction *restrict inst =
	PyObject_New(Instruction, &InstructionType);

    if (inst == NULL) {
	return NULL;
    }

    // TODO How do a get I line of SaMcode?
    inst->inst = "";
    inst->labels = Py_BuildValue("()");
    return (PyObject *) inst;
}

/* PyTypeObject InstructionsIterType {{{2 */
PyTypeObject InstructionsIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "instructionsiterator",
    .tp_basicsize = sizeof (InstructionsIterObject),
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)InstructionsIter_next,
};

/* Instructions {{{1 */
/* typedef Instructions {{{2 */
/* Sequence of the SaM program code */
typedef EsRefObj Instructions;

/* Instructions_iter () {{{2 */
static PyObject *
Instructions_iter(Instructions *restrict self)
{
    InstructionsIterObject *restrict iter =
	PyObject_New(InstructionsIterObject, &InstructionsIterType);

    if (iter == NULL) {
	return NULL;
    }

    iter->es = self->es;
    return (PyObject *)iter;
}

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

/* Module {{{1 */
typedef EsRefObj Module;

/* Module_instructions_get () {{{2 */
static PyObject *
Module_instructions_get(Module *restrict self)
{
    // TODO assumes single module
    Instructions *restrict insts =
	PyObject_New(Instructions, &InstructionsType);

    if (insts == NULL) {
	return NULL;
    }

    insts->es = self->es;
    return (PyObject *) insts;
}

/* Module_filename_get () {{{2 */
static PyObject *
Module_filename_get(Module *restrict self)
{
    // TODO Need to get filename somehow.
    return Py_BuildValue("s", "(ERROR: Filename unknown.)");
}

/* PyGetSetDef Module_getset {{{2 */
static PyGetSetDef Module_getset[] = {
    {"instructions", (getter)Module_instructions_get, NULL,
	"instructions -- the program code of this module.", NULL},
    {"filename", (getter)Module_filename_get, NULL,
	"filename", NULL}
};

/* PyTypeObject ModuleType {{{2 */
static PyTypeObject ModuleType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Module",
    .tp_basicsize = sizeof (Module),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam module (file)",
    .tp_getset	  = Module_getset,
    .tp_new	  = PyType_GenericNew,
};

/* ModulesIter {{{1 */
/* typedef ModulesIterObject {{{2 */
typedef EsRefIterType ModulesIterObject;

/* ModulesIter_next () {{{2 */
static PyObject *
ModulesIter_next(ModulesIterObject *restrict self)
{
    // TODO this is hackish because libsam lacks module support
    if (self->idx != 0)
	return NULL;

    Module *restrict module = PyObject_New(Module, &ModuleType);

    if (module == NULL) {
	return NULL;
    }

    module->es = self->es;
    return (PyObject *) module;
}

/* PyTypeObject ModulesIterType {{{2 */
PyTypeObject ModulesIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "instructionsiterator",
    .tp_basicsize = sizeof (ModulesIterObject),
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)ModulesIter_next,
};


/* Modules {{{1 */
/* typedef Modules {{{2 */
/* Sequence of the SaM program code */
typedef EsRefObj Modules;

/* Modules_iter () {{{2 */
static PyObject *
Modules_iter(Modules *restrict self)
{
    ModulesIterObject *restrict iter =
	PyObject_New(ModulesIterObject, &ModulesIterType);

    if (iter == NULL) {
	return NULL;
    }

    iter->es = self->es;
    return (PyObject *)iter;
}

/* Modules_length {{{2 */
static long
Modules_length(Modules *restrict self)
{
    // TODO this is hackish because libsam lacks module support
    return 1;
}

/* Modules_item {{{2 */
static PyObject *
Modules_item(Modules *restrict self, unsigned i)
{
    // TODO this is hackish because libsam lacks module support
    if(i != 0)
    {
	PyErr_SetNone(PyExc_IndexError);
	return NULL;
    }

    Module *rv = PyObject_New(Module, &ModuleType);
    rv->es = self->es;
    Py_INCREF(rv); // TODO Why is this required?
    return (PyObject *) rv;
}

/* PySquenceMethods Modules_sequence_methods {{{2 */
static PySequenceMethods Modules_sequence_methods = {
    .sq_length = (inquiry)Modules_length,
//    /*(binaryfunc)*/0,			/*sq_concat*/
//    /*(intargfunc)*/0,		/*sq_repeat*/
    .sq_item = (intargfunc)Modules_item,
//    /*(intintargfunc)*/0,		/*sq_slice*/
//    /*(intobjargproc)*/0,		/*sq_ass_item*/
//    /*(intintobjargproc)*/0,	/*sq_ass_slice*/
};

/* PyTypeObject ModulesType {{{2 */
static PyTypeObject ModulesType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Modules",
    .tp_basicsize = sizeof (Modules),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam program code",
    .tp_iter	  = (getiterfunc)Modules_iter,
    .tp_as_sequence = &Modules_sequence_methods,
    .tp_new	  = PyType_GenericNew,
};

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
	case SAM_ML_TYPE_NONE:
	    return PyLong_FromLong(0); // TODO is this right?
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
    return PyLong_FromUnsignedLong(self->value.type);
}

/* PyGetSetDef Value_getset {{{2 */
static PyGetSetDef Value_getset[] = {
    {"value", (getter)Value_value_get, NULL,
	"value -- numerical value (PyLong or PyFloat).", NULL},
    {"type", (getter)Value_type_get, NULL,
	"type -- type of value; index into sam.Types.", NULL}
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
typedef EsRefIterType StackIter;

/* StackIter_next () {{{2 */
static Value *
StackIter_next(StackIter *restrict self)
{
    if (self->idx >= sam_es_stack_len(self->es)) {
	return NULL;
    }

    return Value_create(*sam_es_stack_get(self->es, self->idx++));
}

/* PyTypeObject StackIterType {{{2 */
PyTypeObject StackIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "heapiterator",
    .tp_basicsize = sizeof (StackIter),
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)StackIter_next,
};

/* Stack_iter () {{{2 */
static PyObject *
Stack_iter(PyObject *prog)
{
    StackIter *restrict self =
	PyObject_New(StackIter, &StackIterType);

    if (self == NULL) {
	return NULL;
    }

    self->es = ((Program *)prog)->es;
    self->idx = 0;

    return (PyObject *)self;
}

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
    .tp_as_sequence = &Stack_sequence_methods,
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
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)HeapIter_next,
};

/* Heap_iter () {{{2 */
static PyObject *
Heap_iter(PyObject *prog)
{
    HeapIter *restrict self =
	PyObject_New(HeapIter, &HeapIterType);

    if (self == NULL) {
	return NULL;
    }

    self->es = ((Program *)prog)->es;
    return (PyObject *)self;
}

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
    .tp_as_sequence = &Heap_sequence_methods,
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
    if(err != SAM_OK)
	return NULL;

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

/* Program_stack_get () {{{2 */
static PyObject *
Program_stack_get(Program *restrict self)
{
    Stack *restrict stack = PyObject_New(Stack, &StackType);

    if (stack == NULL) {
	return NULL;
    }

    stack->es = self->es;
    return (PyObject *) stack;
}

/* Program_heap_get () {{{2 */
static PyObject *
Program_heap_get(Program *restrict self)
{
    Heap *restrict heap = PyObject_New(Heap, &HeapType);

    if (heap == NULL) {
	return NULL;
    }

    heap->es = self->es;
    return (PyObject *) heap;
}

/* Program_modules_get () {{{2 */
static PyObject *
Program_modules_get(Program *restrict self)
{
    Modules *restrict modules = PyObject_New(Modules, &ModulesType);

    if (modules == NULL) {
	return NULL;
    }

    modules->es = self->es;
    Py_INCREF(modules); // TODO Why is this required?
    return (PyObject *) modules;
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
    {"stack", (getter)Program_stack_get, NULL,
	"stack.", NULL},
    {"heap", (getter)Program_heap_get, NULL,
	"heap.", NULL},
    {"modules", (getter)Program_modules_get, NULL,
	"the SaM files.", NULL},
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
    
    PyObject *Types = Py_BuildValue("(ssssss)", "none", "int", "float", "sa", "ha", "pa");
    Py_INCREF(Types);
    PyModule_AddObject(m, "Types", Types);

}
