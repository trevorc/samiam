#include <Python.h>
#include "structmember.h"

#include <libsam/sdk.h>

/* locs helper {{{1 */
/* pa_to_dict_key () {{{2 */
static inline PyObject *
pa_to_dict_key(sam_pa pa)
{
    return Py_BuildValue("(ll)", pa.m, pa.l);
}
/* Exceptions {{{1 */
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
	"the labels for this.", NULL},
    {NULL, NULL, NULL, NULL, NULL}
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

/* Instruction_create {{{2 */
static PyObject *
Instruction_create(sam_instruction *restrict si, sam_pa addr,
		   PyObject *locs)
{
    Instruction *restrict rv =
	PyObject_New(Instruction, &InstructionType);
    // TODO we malloc, when do we free?
    char *tmp;
    switch(si->optype) {
	case SAM_OP_TYPE_INT:
	    tmp = malloc(20 + strlen(si->name));
	    /* TODO how big do ints get? */
	    sprintf(tmp, "%s %li", si->name, (long)si->operand.i);
	    rv->inst = tmp;
	    break;
	case SAM_OP_TYPE_FLOAT:
	    tmp = malloc(20 + strlen(si->name));
	    /* TODO how big do floats get? */
	    sprintf(tmp, "%s %f", si->name, si->operand.f);
	    rv->inst = tmp;
	    break;
	case SAM_OP_TYPE_CHAR:
	    tmp = malloc(4 + strlen(si->name));
	    /* TODO are sam_char's really chars? */
	    sprintf(tmp, "%s '%c'", si->name, si->operand.c);
	    rv->inst = tmp;
	    break;
	case SAM_OP_TYPE_LABEL:
	    tmp = malloc(strlen(si->name) + 2 + strlen(si->operand.s));
	    sprintf(tmp, "%s %s", si->name, si->operand.s);
	    rv->inst = tmp;
	    break;
	default:
	    rv->inst = si->name;
    }
    PyObject *key = pa_to_dict_key(addr);
    rv->labels = PyDict_GetItem(locs, key);
    Py_DECREF(key);
    Py_XINCREF(rv->labels);
    if (rv->labels == NULL) {
	rv->labels = Py_BuildValue("()");
    }
    return (PyObject *) rv;
}

/* InstructionsIter {{{1 */
/* typedef InstructionsIterObject {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    unsigned short module_num;
    PyObject *locs;
    size_t idx;
} InstructionsIterObject;

/* InstructionsIter_next () {{{2 */
static PyObject *
InstructionsIter_next(InstructionsIterObject *restrict self)
{
    if (self->idx >= sam_es_instructions_len(self->es, self->module_num)) {
	return NULL;
    }

    Instruction *restrict inst =
	PyObject_New(Instruction, &InstructionType);

    if (inst == NULL) {
	return NULL;
    }
    sam_pa addr = { .m = self->module_num, .l = self->idx++ };
    sam_instruction *si =
	sam_es_instructions_get(self->es, addr);
    return Instruction_create(si, addr, self->locs);
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
typedef struct {
    PyObject_HEAD
    sam_es *es;
    unsigned short module_num;
    PyObject *locs;
} Instructions;

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
    iter->module_num = self->module_num;
    iter->locs = self->locs;
    iter->idx = 0;
    return (PyObject *)iter;
}

/* Instructions_length {{{2 */
static long
Instructions_length(Instructions *self)
{
    return sam_es_instructions_len(self->es, self->module_num);
}

/* Instructions_item {{{2 */
static PyObject *
Instructions_item(Instructions *self, unsigned i)
{
    /* i is unsigned, no check for i < 0 */
    if(i >= sam_es_instructions_len(self->es, self->module_num)) {
	PyErr_SetNone(PyExc_IndexError);
	return NULL;
    }

    sam_pa addr = { .m = self->module_num,.l = i};
    sam_instruction *inst = sam_es_instructions_get(self->es, addr);

    return Instruction_create(inst, addr, self->locs);
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
/* typedef Module {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    PyObject *locs;
    Instructions *insts;
    unsigned short module_num;
} Module;

/* Module_instructions_get () {{{2 */
static PyObject *
Module_instructions_get(Module *restrict self)
{
    if (self->insts == NULL) {
	// TODO assumes single module
	Instructions *restrict insts =
	    PyObject_New(Instructions, &InstructionsType);

	if (insts == NULL) {
	    return NULL;
	}

	insts->es = self->es;
	insts->module_num = 0;
	insts->locs = self->locs;
	self->insts = insts;
    }
    Py_XINCREF(self->insts);
    return (PyObject *)self->insts;
}

/* Module_filename_get () {{{2 */
static PyObject *
Module_filename_get(Module *restrict self)
{
    return PyString_FromString(sam_es_file_get(self->es,
					       self->module_num));
}

/* PyGetSetDef Module_getset {{{2 */
static PyGetSetDef Module_getset[] = {
    {"instructions", (getter)Module_instructions_get, NULL,
	"instructions -- the program code of this module.", NULL},
    {"filename", (getter)Module_filename_get, NULL,
	"filename", NULL},
    {NULL, NULL, NULL, NULL, NULL}
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
typedef struct {
    PyObject_HEAD
    sam_es *es;
    PyObject *locs;
    size_t idx;
} ModulesIterObject;

/* ModulesIter_next () {{{2 */
static PyObject *
ModulesIter_next(ModulesIterObject *restrict self)
{
    if (self->idx >= sam_es_modules_len(self->es)) {
	return NULL;
    }

    Module *restrict module = PyObject_New(Module, &ModuleType);

    if (module == NULL) {
	return NULL;
    }

    module->es = self->es;
    module->insts = NULL;
    module->locs = self->locs;
    module->module_num = self->idx++;
    Py_INCREF(module);
    return (PyObject *) module;
}

/* PyTypeObject ModulesIterType {{{2 */
PyTypeObject ModulesIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "instructionsiterator",
    .tp_basicsize = sizeof (ModulesIterObject),
    .tp_doc	  = "SaM files iterator",
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)ModulesIter_next,
};


/* Modules {{{1 */
/* typedef Modules {{{2 */
/* Sequence of the SaM modules */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    PyObject *locs;
} Modules;

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
    iter->locs = self->locs;
    iter->idx = 0;
    Py_INCREF(iter);
    return (PyObject *)iter;
}

/* Modules_length () {{{2 */
static long
Modules_length(Modules *restrict self)
{
    return sam_es_modules_len(self->es);
}

/* Modules_item () {{{2 */
static PyObject *
Modules_item(Modules *restrict self, unsigned i)
{
    if(i >= sam_es_modules_len(self->es)) {
	PyErr_SetNone(PyExc_IndexError);
	return NULL;
    }

    Module *rv = PyObject_New(Module, &ModuleType);
    rv->es = self->es;
    rv->insts = NULL;
    rv->locs = self->locs;
    rv->module_num = i;
    Py_INCREF(rv);
    return (PyObject *)rv;
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
    .tp_doc	  = "Sam modules (files)",
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
	    return PyLong_FromLong(self->value.value.sa);
	case SAM_ML_TYPE_HA:
	    return PyLong_FromLong(self->value.value.ha);
	case SAM_ML_TYPE_PA:
	    return pa_to_dict_key(self->value.value.pa);
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
	"type -- type of value; index into sam.Types.", NULL},
    {NULL, NULL, NULL, NULL, NULL}
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
    .tp_name	  = "stackiterator",
    .tp_basicsize = sizeof (StackIter),
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)StackIter_next,
};

/* Stack {{{1 */
/* typedef Stack {{{2 */
/* Sequence of the SaM stack */
typedef EsRefObj Stack;

/* Stack_iter () {{{2 */
static PyObject *
Stack_iter(Stack *restrict st)
{
    StackIter *restrict self =
	PyObject_New(StackIter, &StackIterType);

    if (self == NULL) {
	return NULL;
    }

    self->es = st->es;
    self->idx = 0;

    return (PyObject *)self;
}

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

/* Heap {{{1 */
/* typedef Heap {{{2 */
/* Sequence of the SaM heap */
typedef EsRefObj Heap;

/* Heap_iter () {{{2 */
static PyObject *
Heap_iter(Heap *restrict heap)
{
    HeapIter *restrict self =
	PyObject_New(HeapIter, &HeapIterType);

    if (self == NULL) {
	return NULL;
    }

    self->es = heap->es;
    self->idx = 0;

    return (PyObject *)self;
}

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
    .tp_doc	  = "Sam heap",
    .tp_iter	  = (getiterfunc)Heap_iter,
    .tp_as_sequence = &Heap_sequence_methods,
    .tp_new	  = PyType_GenericNew,
};

/* Change {{{1 */
/* typedef Change {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es_change *change;
} Change;

/* Change_type_get () {{{2 */
static PyObject *
Change_type_get(Change *restrict self)
{
    long rv = self->change->stack * 3;
    if (self->change->add)
	rv += 0;
    else if (self->change->remove)
	rv += 1;
    else
	rv += 2;
    return PyLong_FromLong(rv);
}

/* Change_start_get () {{{2 */
static PyObject *
Change_start_get (Change *restrict self)
{
    if (self->change->stack) {
	return PyLong_FromLong(self->change->ma.sa);
    } else {
	return PyLong_FromLong(self->change->ma.ha);
    }
}

/* Change_size_get () {{{2 */
static PyObject *
Change_size_get (Change *restrict self)
{
    return PyLong_FromUnsignedLong(self->change->size);
}

/* Change_value_get () {{{2 */
static PyObject *
Change_value_get (Change *restrict self)
{
    if (self->change->remove) {
	// TODO exception?
	return NULL;
    }

    return (PyObject *) Value_create(*self->change->ml);
}

/* PyGetSetDef Change_getset {{{2 */
static PyGetSetDef Change_getset[] = {
    {"type", (getter)Change_type_get, NULL,
	"type -- type of change; index into sam.ChangeTypes.", NULL},
    {"start", (getter)Change_start_get, NULL,
	"start -- start of change; stack or heap address", NULL},
    {"size", (getter)Change_size_get, NULL,
	"size -- size of change in sam words", NULL},
    {"value", (getter)Change_value_get, NULL,
	"value -- sam.Value of change if add or modify", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

/* Change_dealloc () {{{2 */
static void
Change_dealloc(Change *self)
{
    if (self->change) {
	free(self->change);
    }
    self->ob_type->tp_free((PyObject *)self);
}

/* PyTypeObject ChangeType {{{2 */
static PyTypeObject ChangeType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Change",
    .tp_basicsize = sizeof (Change),
    .tp_dealloc	  = (destructor)Change_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "One change in the sam memory state",
    .tp_getset	  = Change_getset,
    .tp_new	  = PyType_GenericNew,
};

/* ChangesIter {{{1 */
/* typedef ChangesIter {{{2 */
typedef EsRefObj ChangesIter;

/* ChangesIter_next () {{{2 */
static PyObject *
ChangesIter_next(ChangesIter *restrict self)
{
    sam_es_change *ch = malloc(sizeof(sam_es_change));
    if (sam_es_change_get(self->es, ch)) {
	Change *rv = PyObject_New(Change, &ChangeType);
	rv->change = ch;
	
	return (PyObject *)rv;
    } else {
	free(ch);
	return NULL;
    }
}

/* PyTypeObject ChangesIterType {{{2 */
PyTypeObject ChangesIterType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name	  = "changesiterator",
    .tp_basicsize = sizeof (ChangesIter),
    .tp_free	  = PyObject_Free,
    .tp_getattro  = PyObject_GenericGetAttr,
    .tp_flags	  = Py_TPFLAGS_DEFAULT,
    .tp_iter	  = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)ChangesIter_next,
};

/* Changes {{{1 */
/* typedef Changes {{{2 */
/* Sequence of the SaM changes */
typedef EsRefObj Changes;

/* Changes_iter () {{{2 */
static PyObject *
Changes_iter(Changes *restrict ch)
{
    ChangesIter *restrict self =
	PyObject_New(ChangesIter, &ChangesIterType);

    if (self == NULL) {
	return NULL;
    }

    self->es = ch->es;

    return (PyObject *)self;
}

/* PyTypeObject ChangesType {{{2 */
static PyTypeObject ChangesType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Changes",
    .tp_basicsize = sizeof (Changes),
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam memory changes",
    .tp_iter	  = (getiterfunc)Changes_iter,
    .tp_new	  = PyType_GenericNew,
};

/* Program {{{1 */
/* typedef Program {{{2 */
typedef struct {
    PyObject_HEAD
    sam_es *es;
    char *file;
    Stack *stack;
    Heap *heap;
    Modules *modules;
    PyObject *locs;
    PyObject *print_func;
    PyObject *input_func;
} Program;

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

/* Program_lc_get () {{{2 */
static PyObject *
Program_lc_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_pc_get(self->es).l);
}

/* Program_lc_set () {{{2 */
static int
Program_lc_set(Program *restrict self, PyObject *v)
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
    sam_es_pc_set(self->es, (sam_pa) {
	.m = sam_es_pc_get(self->es).m,
	.l = PyLong_AsLong(v)
    });

    return 0;
}

/* Program_mc_get () {{{2 */
static PyObject *
Program_mc_get(Program *restrict self)
{
    return PyLong_FromUnsignedLong(sam_es_pc_get(self->es).m);
}

/* Program_mc_set () {{{2 */
static int
Program_mc_set(Program *restrict self, PyObject *v)
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
    sam_es_pc_set(self->es, (sam_pa) { .l = sam_es_pc_get(self->es).l,
		  .m = PyLong_AsLong(v)});

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
    if (self->stack == NULL) {
	Stack *restrict stack = PyObject_New(Stack, &StackType);

	if (stack == NULL) {
	    return NULL;
	}

	stack->es = self->es;
	self->stack = stack;
    }
    Py_XINCREF(self->stack);
    return (PyObject *)self->stack;
}

/* Program_heap_get () {{{2 */
static PyObject *
Program_heap_get(Program *restrict self)
{
    if (self->heap == NULL) {
	Heap *restrict heap = PyObject_New(Heap, &HeapType);

	if (heap == NULL) {
	    return NULL;
	}

	heap->es = self->es;
	self->heap = heap;
    }
    Py_XINCREF(self->heap);
    return (PyObject *)self->heap;
}

/* Program_modules_get () {{{2 */
static PyObject *
Program_modules_get(Program *restrict self)
{
    if (self->modules == NULL)
    {
	Modules *restrict modules = PyObject_New(Modules, &ModulesType);

	if (modules == NULL) {
	    return NULL;
	}

	modules->es = self->es;
	modules->locs = self->locs;
	self->modules = modules;
    }
    Py_XINCREF(self->modules);
    return (PyObject *)self->modules;
}

/* Program_changes_get () {{{2 */
static PyObject *
Program_changes_get(Program *restrict self)
{
    Changes *restrict changes = PyObject_New(Changes, &ChangesType);

    if (changes == NULL) {
	return NULL;
    }

    changes->es = self->es;
    Py_INCREF(changes); // TODO Why is this required?
    return (PyObject *)changes;
}

/* Program_print_func_get () {{{2 */
static PyObject *
Program_print_func_get(Program *restrict self)
{
    PyObject *restrict rv = self->print_func == NULL
	? Py_None : self->print_func;
    Py_INCREF(rv);
    return rv;
}

/* Program_print_func_set () {{{2 */
static int
Program_print_func_set(Program *restrict self, PyObject *func)
{
    // If func is null, the print_func is cleared.
    if (func != NULL && !PyCallable_Check(func)) {
	PyErr_SetString(PyExc_TypeError, "print_func must be callable");
	return -1;
    }
    Py_XDECREF(self->print_func);
    Py_XINCREF(func);
    self->print_func = func;

    return 0;
}

/* Program_input_func_get () {{{2 */
static PyObject *
Program_input_func_get(Program *restrict self)
{
    PyObject *restrict rv = self->input_func == NULL
	? Py_None : self->input_func;
    Py_INCREF(rv);
    return rv;
}

/* Program_input_func_set () {{{2 */
static int
Program_input_func_set(Program *restrict self, PyObject *func)
{
    // If func is null, the input_func is cleared.
    if (func != NULL && !PyCallable_Check(func)) {
	PyErr_SetString(PyExc_TypeError, "input_func must be callable");
	return -1;
    }
    Py_XDECREF(self->input_func);
    Py_XINCREF(func);
    self->input_func = func;

    return 0;
}

/* PyGetSetDef Program_getset {{{2 */
static PyGetSetDef Program_getset[] = {
    {"bt", (getter)Program_bt_get, (setter)Program_bt_set,
	"back trace bit.", NULL},
    {"fbr", (getter)Program_fbr_get, (setter)Program_fbr_set,
	"frame base register.", NULL},
    {"mc", (getter)Program_mc_get, (setter)Program_mc_set,
	"module counter -- module number of current execution.", NULL},
    {"lc", (getter)Program_lc_get, (setter)Program_lc_set,
	"line counter -- line number within module.", NULL},
    {"sp", (getter)Program_sp_get, NULL,
	"stack pointer.", NULL},
    {"stack", (getter)Program_stack_get, NULL,
	"stack.", NULL},
    {"heap", (getter)Program_heap_get, NULL,
	"heap.", NULL},
    {"modules", (getter)Program_modules_get, NULL,
	"the sam files.", NULL},
    {"changes", (getter)Program_changes_get, NULL,
	"sam memory changes.", NULL},
    {"print_func", (getter)Program_print_func_get,
	(setter)Program_print_func_set,
	"function for sam program to use to print a string", NULL},
    {"input_func", (getter)Program_input_func_get,
	(setter)Program_input_func_set,
	"function for sam program to use to get a string input", NULL},
    {NULL, NULL, NULL, NULL, NULL}
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
    Py_XDECREF(self->heap);
    Py_XDECREF(self->stack);
    Py_XDECREF(self->modules);
    Py_XDECREF(self->locs);
}

/* Program_step () {{{2 */
static PyObject *
Program_step(Program *restrict self)
{
    // TODO multi-modules
    long err = sam_es_instructions_cur(self->es)->handler(self->es);
    if(err == SAM_STOP) {
	Py_RETURN_FALSE;
    } else if(err != SAM_OK) {
	PyErr_SetObject(SamError, PyInt_FromLong(err));
	return NULL;
    }

    sam_es_pc_pp(self->es);

    Py_RETURN_TRUE;
}

/* Program IO funcs {{{2 */
/* Program_io_vfprintf_func () {{{3 */
static int
Program_io_vfprintf(sam_io_stream ios,
		    void *data,
		    const char *restrict fmt,
		    va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    int len = vsnprintf(NULL, 0, fmt, ap);
    char *str = malloc(len);
    vsprintf(str, fmt, aq);
    va_end(aq);
    PyObject *restrict arglist = Py_BuildValue("(s)", str);
    Program *restrict self = data;
    PyEval_CallObject(self->print_func, arglist);
    Py_DECREF(arglist);
    return len;
}

/* Program_io_afgets_func () {{{3 */
static char *
Program_io_afgets(char **restrict s,
		  sam_io_stream ios,
		  void *data)
{
    PyObject *restrict arglist = Py_BuildValue("()");
    Program *restrict self = data;
    PyObject *res = PyEval_CallObject(self->input_func, arglist);
    Py_DECREF(arglist);
    *s = strdup(PyString_AsString(res));
    Py_DECREF(res);
    return *s;
}

/* Program_io_dispatcher () {{{3 */
static sam_io_func
Program_io_dispatcher(sam_io_func_name io_func, void *data)
{
    Program *restrict self = data;
    switch(io_func) {
	case SAM_IO_VFPRINTF:
	    return self->print_func == NULL?
		(sam_io_func){ NULL }:
		(sam_io_func){.vfprintf = Program_io_vfprintf};
	case SAM_IO_AFGETS:
	    return self->input_func == NULL?
		(sam_io_func){ NULL }:
		(sam_io_func){.afgets = Program_io_afgets};
	default:
	    return (sam_io_func){ NULL };
    }
}

/* Program_load () {{{2 */
static int
Program_load(Program *restrict self)
{
    self->es = sam_es_new(self->file, 1, Program_io_dispatcher, self);
    if (self->es == NULL) {
	PyErr_SetString(ParseError, "couldn't parse input file.");
	return -1;
    }

    self->locs = PyDict_New();
    sam_array *locs = sam_es_locs_get(self->es);
    for (unsigned i = 0; i < locs->len; i++) {
	sam_es_loc *loc = locs->arr[i];
	PyObject *labels = PyTuple_New(loc->labels.len);
	for (unsigned j = 0; j < loc->labels.len; j++) {
	    // TODO ref owner of the string?
	    PyTuple_SetItem(labels, j,
			    PyString_FromString(loc->labels.arr[j]));
	}
	PyObject *key = pa_to_dict_key(loc->pa);
	PyDict_SetItem(self->locs, key, labels);
	Py_DECREF(key);
    }
    Py_INCREF(self->locs);

    return 0;
}

/* Program_reset () {{{2 */
static PyObject *
Program_reset(Program *restrict self)
{
    sam_es_reset(self->es);
    Py_RETURN_TRUE;
}

/* PyMethodDef Program_methods {{{2 */
static PyMethodDef Program_methods[] = {
    {"step", (PyCFunction)Program_step, METH_NOARGS,
	"Steps one instruction; returns true if the program continues"},
    {"reset", (PyCFunction)Program_reset, METH_NOARGS,
	"Resets program to the beginning. Note this also resets changes."},
    {0, 0, 0, 0}, /* Sentinel */
};

/* Program_init () {{{2 */
static int
Program_init(Program *restrict self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"file", NULL};
    char *file;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &file)) {
	return -1; 
    }
    self->file = strdup(file);
    self->heap = NULL;
    self->stack = NULL;
    self->locs = NULL;
    self->modules = NULL;
    self->print_func = NULL;
    self->input_func = NULL;

    return Program_load(self);
}

/* PyTypeObject ProgramType {{{2 */
static PyTypeObject ProgramType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name	  = "sam.Program",
    .tp_basicsize = sizeof (Program),
    .tp_dealloc   = (destructor)Program_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc	  = "Sam execution state",
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
    if (PyType_Ready(&ValueType) < 0 ||
	PyType_Ready(&InstructionType) < 0 ||
	PyType_Ready(&InstructionsIterType) < 0 ||
	PyType_Ready(&InstructionsType) < 0 ||
	PyType_Ready(&StackIterType) < 0 ||
	PyType_Ready(&StackType) < 0 ||
	PyType_Ready(&HeapIterType) < 0 ||
	PyType_Ready(&HeapType) < 0 ||
	PyType_Ready(&ModuleType) < 0 ||
	PyType_Ready(&ModulesType) < 0 ||
	PyType_Ready(&ModulesIterType) < 0 ||
	PyType_Ready(&ChangeType) < 0 ||
	PyType_Ready(&ChangesIterType) < 0 ||
	PyType_Ready(&ChangesType) < 0 ||
	PyType_Ready(&ProgramType) < 0) {
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
    PyModule_AddObject(m, "Program", (PyObject *)&ProgramType);
    
    PyObject *Types = Py_BuildValue("(ssssss)", "none", "int",
				    "float", "sa", "ha", "pa");
    Py_INCREF(Types);
    PyModule_AddObject(m, "Types", Types);

    PyObject *TypeChars = Py_BuildValue("(ssssss)", "N", "I", "F", "S",
					"H", "P");
    Py_INCREF(TypeChars);
    PyModule_AddObject(m, "TypeChars", TypeChars);

    PyObject *ChangeTypes = Py_BuildValue("(ssssss)",
			    "heap_alloc", "heap_free", "heap_change",
			    "stack_push", "stack_pop", "stack_change");
    Py_INCREF(ChangeTypes);
    PyModule_AddObject(m, "ChangeTypes", ChangeTypes);

    PyObject *Errors = Py_BuildValue("(ssssssssssssssssss)",
			    "OK", "Stop", "Unexpected optype",
			    "Segmentation fault", "Illegal free",
			    "Stack underflow", "Stack overflow",
			    "Out of memory", "Illegal type conversion",
			    "Too many elements on final stack",
			    "Unknown identifier",
			    "Illegal stack input type", "I/O error",
			    "Division by zero", "Negative shift",
			    "Unsupported opcode",
			    "Failed to load dynamic linking",
			    "Dynamic linking symbol "
				"could not be resolved");
    Py_INCREF(Errors);
    PyModule_AddObject(m, "Errors", Errors);
}
