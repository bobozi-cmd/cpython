
/* MyDebug object interface */

#ifndef Py_MYDEBUGOBJECT_H
#define Py_MYDEBUGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
	typedef struct {
		PyObject_HEAD

		/* Attributes dictionary */
		PyObject* step;
	
	} PyMyDebugObject;
#endif

#define PyMyDebug_Check(op) \
    PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_DEFAULT)

/* PUBLIC VAR */
PyAPI_DATA(PyTypeObject) PyMyDebug_Type;
/* flag to show my debug mode*/
PyAPI_DATA(Py_ssize_t)   my_debug;
/* flag to show my debug mode*/

#define _Py_MYDEBUG_ON_LONG    my_debug = 4
#define _Py_MYDEBUG_ON_LIST    my_debug = 3
#define _Py_MYDEBUG_ON_DICT    my_debug = 2
#define _Py_MYDEBUG_ON_ALL     my_debug = 1
#define _Py_MYDEBUG_OFF        my_debug = 0

#define Py_MYDEBUG_LONG 4
#define Py_MYDEBUG_LIST 3
#define Py_MYDEBUG_DICT 2
#define Py_MYDEBUG_ALL  1
#define Py_MYDEBUG_OFF  0

/* trace longobject's create operation*/
#define _Py_MY_LONG_CREATE_TRACE_ON     my_long_create_step = 0
#define _Py_MY_LONG_CREATE_TRACE_OFF    my_long_create_step = -1
#define _Py_MY_LONG_CREATE_TRACE_NEXT    my_long_create_step++
/* if my_long_create_step > -1, then trace mode on*/
#define Py_MYLONG_CREATE_TRACE_OFF -1


/* FUNCTION API */
PyAPI_FUNC(PyObject *)       PyMyDebug_New();
PyAPI_FUNC(void) Py_MyDebug_Switch(PyObject* cmd);
PyAPI_FUNC(Py_ssize_t) _Py_GetMy_Debug(void);
PyAPI_FUNC(void) Py_MyDebug_Small_Ints(char* func_name, PyObject* v, PyLongObject small_ints[]);

/* trace longobject's create operation*/
PyAPI_DATA(Py_ssize_t) my_long_create_step;
PyAPI_FUNC(Py_ssize_t) _Py_GetMy_Long_Create_Step(void);
PyAPI_FUNC(void) Py_MyDebug_long_frombytes(PyObject * obj);
PyAPI_FUNC(void) Py_MyDebug_long_fromunicodeobject(PyObject * obj);
PyAPI_FUNC(void) Py_MyDebug_long_fromlong(void);
PyAPI_FUNC(void) Py_MyDebug_longobject_create(PyTypeObject *type, PyObject *x, PyObject *obase);
PyAPI_FUNC(void) Py_MyDebug_number_long();

/* trace listobject's create operation*/
PyAPI_FUNC(void) Py_MyDebug_List_Create(Py_ssize_t size, PyListObject *op);
PyAPI_FUNC(void) Py_MyDebug_List_Print(PyListObject* op);
PyAPI_FUNC(void) Py_MyDebug_List_Init(PyListObject* op);
PyAPI_FUNC(void) Py_MyDebug_List_Setitem(PyObject* op);
PyAPI_FUNC(void) Py_MyDebug_List_Appenditem(PyListObject* obj, PyObject* new_item);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MYDEBUGOBJECT_H */
