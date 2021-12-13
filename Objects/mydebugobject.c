/* MyDebug object implementation */

#include "Python.h"
#include "mydebugobject.h"
#include "internal/pystate.h"
#include "accu.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>          /* For size_t */
#endif

PyTypeObject PyMyDebug_Type;


/* PyObject */
Py_ssize_t my_debug = Py_MYDEBUG_OFF;

Py_ssize_t
_Py_GetMy_Debug(void) {
	Py_ssize_t mode = my_debug;
	return mode;
}

PyObject* PyMyDebug_New();

void Py_MyDebug_Switch(PyObject* cmd);

/* PyLongObject */
Py_ssize_t my_long_create_step = Py_MYLONG_CREATE_TRACE_OFF;

Py_ssize_t
_Py_GetMy_Long_Create_Step(void) {
	Py_ssize_t step = my_long_create_step;
	return step;
}

void Py_MyDebug_Small_Ints(char* func_name, PyObject* v, PyLongObject small_ints[]);

void Py_MyDebug_long_frombytes(PyObject * obj);

void Py_MyDebug_long_fromunicodeobject(PyObject * obj);

void Py_MyDebug_long_fromlong(void);

void Py_MyDebug_longobject_create(PyTypeObject *type, PyObject *x, PyObject *obase);

void Py_MyDebug_number_long();

/* PyListObject */

void Py_MyDebug_List_Create(Py_ssize_t size, PyListObject *op);

void Py_MyDebug_List_Print(PyListObject* op);

void Py_MyDebug_List_Init(PyListObject* op);

void Py_MyDebug_List_Setitem(PyObject* op);

void Py_MyDebug_List_Appenditem(PyListObject* obj, PyObject* new_item);

PyObject*
PyMyDebug_New()
{
	
	PyMyDebugObject *self;
	self = PyObject_New(PyMyDebugObject, &PyMyDebug_Type);
	if (self == NULL)
		return NULL;
	self->step = NULL;
	return (PyObject*)self;

}

static void
MyDebug_dealloc(PyMyDebugObject *self)
{
	Py_XDECREF(self->step);
	PyObject_Del(self);
}

static PyObject *
MyDebug_demo(PyMyDebugObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":demo"))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef MyDebug_methods[] = {
	{
		"demo",            
		(PyCFunction)MyDebug_demo,  
		METH_VARARGS,
		PyDoc_STR("demo() -> None")
	},
	{
		NULL,              
		NULL
	}           /* sentinel */
};

static PyObject *
MyDebug_getattro(PyMyDebugObject *self, PyObject *name)
{
	if (self->step != NULL) {
		PyObject *v = PyDict_GetItem(self->step, name);
		if (v != NULL) {
			Py_INCREF(v);
			return v;
		}
	}
	return PyObject_GenericGetAttr((PyObject *)self, name);
}

static int
MyDebug_setattr(PyMyDebugObject *self, char *name, PyObject *v)
{
	if (self->step == NULL) {
		self->step = PyDict_New();
		if (self->step == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(self->step, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
				"delete non-existing MyObj attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(self->step, name, v);
}


PyTypeObject MyObj_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"myDebug",             /*tp_name*/
	sizeof(PyMyDebugObject),          /*tp_basicsize*/
	0,                          /*tp_itemsize*/
	/* methods */
	(destructor)MyDebug_dealloc,    /*tp_dealloc*/
	0,                          /*tp_print*/
	(getattrfunc)0,             /*tp_getattr*/
	(setattrfunc)MyDebug_setattr,   /*tp_setattr*/
	0,                          /*tp_reserved*/
	0,                          /*tp_repr*/
	0,                          /*tp_as_number*/
	0,                          /*tp_as_sequence*/
	0,                          /*tp_as_mapping*/
	0,                          /*tp_hash*/
	0,                          /*tp_call*/
	0,                          /*tp_str*/
	(getattrofunc)MyDebug_getattro, /*tp_getattro*/
	0,                          /*tp_setattro*/
	0,                          /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,         /*tp_flags*/
	0,                          /*tp_doc*/
	0,                          /*tp_traverse*/
	0,                          /*tp_clear*/
	0,                          /*tp_richcompare*/
	0,                          /*tp_weaklistoffset*/
	0,                          /*tp_iter*/
	0,                          /*tp_iternext*/
	MyDebug_methods,                /*tp_methods*/
	0,                          /*tp_members*/
	0,                          /*tp_getset*/
	0,                          /*tp_base*/
	0,                          /*tp_dict*/
	0,                          /*tp_descr_get*/
	0,                          /*tp_descr_set*/
	0,                          /*tp_dictoffset*/
	0,                          /*tp_init*/
	0,                          /*tp_alloc*/
	0,                          /*tp_new*/
	0,                          /*tp_free*/
	0,                          /*tp_is_gc*/
};
/* --------------------------------------------------------------------- */

/*
	open or close or change my debug mode
*/
static void
_Py_Switch_Debug_Mode(PyObject* cmd)
{
	/* start my debug mode by print("myDebugOnxxx") */
	if (PyUnicode_Check(cmd)) {

		PyObject* cmp_on_long = PyUnicode_FromString("myDebugOnLong");
		PyObject* cmp_on_list = PyUnicode_FromString("myDebugOnList");
		PyObject* cmp_on_dict = PyUnicode_FromString("myDebugOnDict");
		PyObject* cmp_on_all = PyUnicode_FromString("myDebugOnAll");
		PyObject* cmp_off = PyUnicode_FromString("myDebugOff");

		if (PyUnicode_Compare(cmp_on_long, cmd) == 0) {
			printf("<my_debug is %d\n", my_debug);
			printf("[myDebug mode on]\n");
			_Py_MYDEBUG_ON_LONG;
			printf("my_debug become %d>\n", my_debug);
		}
		else if (PyUnicode_Compare(cmp_on_list, cmd) == 0) {
			printf("<my_debug is %d\n", my_debug);
			printf("[myDebug mode on]\n");
			_Py_MYDEBUG_ON_LIST;
			printf("my_debug become %d>\n", my_debug);
		}
		else if (PyUnicode_Compare(cmp_on_dict, cmd) == 0) {
			printf("<my_debug is %d\n", my_debug);
			printf("[myDebug mode on]\n");
			_Py_MYDEBUG_ON_DICT;
			printf("my_debug become %d>\n", my_debug);
		}
		else if (PyUnicode_Compare(cmp_on_all, cmd) == 0) {
			printf("<my_debug is %d\n", my_debug);
			printf("[myDebug mode on]\n");
			_Py_MYDEBUG_ON_ALL;
			printf("my_debug become %d>\n", my_debug);
		}
		else if (PyUnicode_Compare(cmp_off, cmd) == 0) {
			printf("<my_debug is %d\n", my_debug);
			printf("[myDebug mode close]\n");
			_Py_MYDEBUG_OFF;
			printf("my_debug become %d>\n", my_debug);
		}

		Py_XDECREF(cmp_on_long);
		Py_XDECREF(cmp_on_list);
		Py_XDECREF(cmp_on_dict);
		Py_XDECREF(cmp_on_all);
		Py_XDECREF(cmp_off);
	}
	/* end my debug mode by print("myDebugOff") */
}

void
Py_MyDebug_Switch(PyObject* cmd)
{
	if (cmd == NULL) {
		return;
	}
	_Py_Switch_Debug_Mode(cmd);
}

static void
longobject_small_ints(char* func_name, PyObject* v, PyLongObject small_ints[])
{

	if (_Py_GetMy_Debug() == Py_MYDEBUG_LONG || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		int values[10];
		int refcounts[10];

		printf("[%s]\n", func_name);
		// printf("address is @%p\n", v);
		// 考察小整数对象池的信息
		for (int i = 0; i < 10; ++i)
		{
			if (small_ints[i].ob_base.ob_size == 0)
			{
				values[i] = 0;
			}
			else
			{
				values[i] = small_ints[i].ob_digit[0];
				if (i <= 4)
				{
					values[i] *= -1;
				}
			}
			refcounts[i] = small_ints[i].ob_base.ob_base.ob_refcnt;
		}
		printf("  value : ");
		for (int i = 0; i < 8; ++i)
		{
			printf("%d\t", values[i]);
		}
		printf("\n");
		printf(" refcnt : ");
		for (int i = 0; i < 8; ++i)
		{
			printf("%d\t", refcounts[i]);
		}
		printf("\n");
		printf("[%s]\n", func_name);
	}

}

void 
Py_MyDebug_Small_Ints(char* func_name, PyObject* v, PyLongObject small_ints[])
{
	PyObject *a, *min, *max;

	a   = v;
	min = PyLong_FromLong(-10);
	max = PyLong_FromLong(15);

	if (a == NULL || !PyLong_Check(a))
	{
		PyErr_BadInternalCall();
		return;
	}

	if (PyObject_RichCompareBool(a, max, Py_LE) != 1 || PyObject_RichCompareBool(a, min, Py_GE) != 1)
	{
		/* control num which can trigger this function between -10 and 15*/
		Py_XDECREF(min);
		Py_XDECREF(max);
		return;
	}

	longobject_small_ints(func_name, v, small_ints);

	Py_XDECREF(min);
	Py_XDECREF(max);

}

static void
longobject_frombytes_trace(PyObject * obj)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LONG || _Py_GetMy_Debug() == Py_MYDEBUG_ALL) {
		if (_Py_GetMy_Long_Create_Step() > -1) {
			printf("[%d] come into _PyLong_FromBytes() and create a new PyLongObject\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
			if (PyLong_Check(obj)) {
				PyLongObject* mlong = (PyLongObject*)obj;
				printf("[%d] _PyLong_FromBytes() create a new PyLongObject by PyLong_FromString()\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
				printf("new PyLongObject's attribute is:\n");
				printf("\tmlong->ob_size -> %d\n", mlong->ob_base.ob_size);
				for (int j = 0; j < abs(mlong->ob_base.ob_size); j++) {
					printf("\tob_digit[%d]:%d * (2^PYLONG_BITS_IN_DIGIT)^%d ", j, mlong->ob_digit[j], j);
					if (j != abs(mlong->ob_base.ob_size) - 1) {
						printf("+");
					}
					printf("\n");
				}
				printf("===digit's size is %d and 2^PYLONG_BITS_IN_DIGIT is %d===\n", sizeof(digit), (int)pow(2, PYLONG_BITS_IN_DIGIT));

			}
			else if (PyBytes_Check(obj))
			{
				printf("[%d] _PyLong_FromBytes() create a new PyBytesObject by PyBytes_FromStringAndSize()\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
				printf("then return NULL if no error occurs\n");
			}
		}
		_Py_MY_LONG_CREATE_TRACE_OFF;
	}
}

void 
Py_MyDebug_long_frombytes(PyObject * obj)
{
	if (obj == NULL){
		return;
	}
	longobject_frombytes_trace(obj);
}

static void
longobject_fromunicodeobject_trace(PyObject * obj)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LONG || _Py_GetMy_Debug() == Py_MYDEBUG_ALL) {
		if (_Py_GetMy_Long_Create_Step() > -1) {
			printf("[%d] come into PyLong_FromUnicodeObject() and create a new PyLongObject\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
		}
		_Py_MY_LONG_CREATE_TRACE_OFF;
	}
}

void
Py_MyDebug_long_fromunicodeobject(PyObject * obj) 
{
	if (obj == NULL) {
		return;
	}
	longobject_fromunicodeobject_trace(obj);
}

static void
longobject_fromlong_trace()
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LONG || _Py_GetMy_Debug() == Py_MYDEBUG_ALL) {
		if (_Py_GetMy_Long_Create_Step() > -1) {
			printf("[%d] come into PyLong_FromLong() and create a new PyLongObject\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
		}
		_Py_MY_LONG_CREATE_TRACE_OFF;
	}
}

void
Py_MyDebug_long_fromlong()
{
	longobject_fromlong_trace();
}

static void
longobject_create(PyTypeObject *type, PyObject *x, int base)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LONG || _Py_GetMy_Debug() == Py_MYDEBUG_ALL) {

		_Py_MY_LONG_CREATE_TRACE_ON;

		printf("[%d]int() will call method long_new_impl(PyTypeObject *type, PyObject *x, PyObject *obase)\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
		printf("you input attribute is :\n");
		printf("\ttype -> %s\n", type->tp_name);
		printf("\tx -> %s\n", (x == NULL) ? "NULL" : "Object");
		if (base == NULL) {
			printf("\tobase -> NULL\n");
		}
		else {
			printf("\tobase -> %d\n", base);
		}

		if (type != &PyLong_Type) {
			printf("[%d]input type is not PyLong_Type, \
				then we will use long_subtype_new() check it whether a subtype of PyLong_Type\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
		}
		printf("[%d]we will choose a function to create a new PyLong_Object\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
		printf("\t1.PyLong_FromLong()         ---  x==NULL and obase==NULL\n");
		printf("\t2.PyNumber_Long()           ---  x!=NULL and obase==NULL\n");
		printf("\t3.PyLong_FromUnicodeObject  ---  PyUnicode_Check(x)\n");
		printf("\t4._PyLong_FromBytes()       ---  PyByteArray_Check(x) or PyBytes_Check(x)\n");
	}
}

void
Py_MyDebug_longobject_create(PyTypeObject *type, PyObject *x, PyObject *obase)
{
	int base = NULL;
	
	if (obase != NULL) {
		base = PyNumber_AsSsize_t(obase, NULL);
		if (base == -1 && PyErr_Occurred()) {
			return;
		}
	}

	longobject_create(type, x, base);
}

static void
longobject_fromnumber_trace()
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LONG || _Py_GetMy_Debug() == Py_MYDEBUG_ALL) {
		if (_Py_GetMy_Long_Create_Step() > -1) {
			printf("[%d] come into [abstract.c]PyNumber_Long() and create a new PyLongObject\n", _Py_MY_LONG_CREATE_TRACE_NEXT);
		}
		_Py_MY_LONG_CREATE_TRACE_OFF;
	}
}


void
Py_MyDebug_number_long()
{
	longobject_fromnumber_trace();
}

static void
list_print(PyListObject *op)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LIST || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		PyObject_Print((PyObject*)op, stdout, 1);
		printf("\n[");
		for (int i = 0; i < op->ob_base.ob_size; i++) {
			/* please add item into list expect Int and String */
			printf("%p", op->ob_item[i]);
			if (i != op->ob_base.ob_size - 1) {
				printf(", ");
			}
		}
		printf("]\n");
	}
}

static void
listobject_create(Py_ssize_t size, PyListObject *op)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LIST || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		/*
			aim to reduce info print, I limit that if list's size >= 3, then print list's info
			because when we print a info and system do some preprocess, Cpython internal will new
			some temp list to store some extra info
		 */
		 
		if (size >= 3)
		{

			printf("[trace_list_new]\n");
			printf("new list's ob_size=%d, ", op->ob_base.ob_size);
			printf("allocated=%d, ", op->allocated);
			printf("ob_item.address @%p\n", op->ob_item);

			list_print(op);

			printf("[trace_list_new]\n");
		}
	}
}

void
Py_MyDebug_List_Create(Py_ssize_t size, PyListObject *op)
{
	listobject_create(size, op);
}

void
Py_MyDebug_List_Print(PyListObject* op)
{
	if (op == NULL) {
		return;
	}
	list_print(op);
}

static void
list_init_trace(PyListObject *op)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LIST || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		printf("[trace_list_init]\n");
		list_print(op);
		printf("[trace_list_init]\n");
	}
}

void
Py_MyDebug_List_Init(PyListObject* op)
{
	if (op == NULL) {
		return;
	}
	list_init_trace(op);
}

static void
list_setitem_trace(PyObject* obj)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LIST || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		printf("[PyList_SetItem]\n");
		printf("now set item:");
		PyObject_Print(obj, stdout, 1);
		printf("\n[PyList_SetItem]\n");
	}
}

void
Py_MyDebug_List_Setitem(PyObject* op)
{
	if (op == NULL) {
		return;
	}
	list_setitem_trace(op);
}

static void
list_appenditem_trace(PyListObject* obj, PyObject* new_item)
{
	if (_Py_GetMy_Debug() == Py_MYDEBUG_LIST || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		/* control input */
		PyObject* app = PyUnicode_FromString("append");

		if (PyByteArray_Check(new_item) || PyBytes_Check(new_item) || PyUnicode_Check(new_item)) {

			if (PyObject_RichCompareBool(app, new_item, Py_EQ) == 1) {
				printf("[list_append]\n");
				printf("now set item: ");
				PyObject_Print(new_item, stdout, 1);
				printf("\n");

				printf("new list's ob_size=%d, ", obj->ob_base.ob_size);
				printf("allocated=%d, ", obj->allocated);
				printf("ob_item.address @%p\n", obj->ob_item);

				list_print(obj);  /* print list elems addresses */ 

				printf("[list_append]\n");
			}
		}


	}
}

void
Py_MyDebug_List_Appenditem(PyListObject* obj, PyObject* new_item)
{
	if (obj == NULL || new_item == NULL) {
		return;
	}
	list_appenditem_trace(obj, new_item);
}
