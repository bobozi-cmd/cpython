
/* List object interface */

/*
Another generally useful object type is a list of object pointers.
This is a mutable type: the list items can be changed, and items can be
added or removed.  Out-of-range indices or non-list objects are ignored.

另一种通常有用的对象类型是对象指针列表。
这是一种可变类型：列表项可以更改，项可以添加或删除。将忽略超出范围的索引或非列表对象。

*** WARNING *** PyList_SetItem does not increment the new item's reference
count, but does decrement the reference count of the item it replaces,
if not nil.  It does *decrement* the reference count if it is *not*
inserted in the list.  Similarly, PyList_GetItem does not increment the
returned item's reference count.

PyList_SetItem不会增加新项目的引用计数，但会减少其替换项的引用计数（如果不是nil）
如果它*未*插入列表，则它会*减少*引用计数。类似地，
PyList_GetItem不会增加返回项目的引用计数。
*/

#ifndef Py_LISTOBJECT_H
#define Py_LISTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
typedef struct {
    PyObject_VAR_HEAD
    /* Vector of pointers to list elements.  list[0] is ob_item[0], etc. */
    PyObject **ob_item;

    /* ob_item contains space for 'allocated' elements.  The number
     * currently in use is ob_size.
     * Invariants:
     *     0 <= ob_size <= allocated
     *     len(list) == ob_size
     *     ob_item == NULL implies ob_size == allocated == 0
     * list.sort() temporarily sets allocated to -1 to detect mutations.
     *
     * Items must normally not be NULL, except during construction when
     * the list is not yet visible outside the function that builds it.

	   ob_item包含的是已经分配元素的空间,ob_size为当前元素数量
	     0 <= ob_size <= allocated
		 len(list) == ob_size
		 ob_item == NULL implies ob_size == allocated == 0
	   Items通常不为空。除非在构造期间，此时列表在构造它的函数之外还不可见。

     */
    Py_ssize_t allocated;
} PyListObject;
#endif

PyAPI_DATA(PyTypeObject) PyList_Type;
PyAPI_DATA(PyTypeObject) PyListIter_Type;
PyAPI_DATA(PyTypeObject) PyListRevIter_Type;
PyAPI_DATA(PyTypeObject) PySortWrapper_Type;

#define PyList_Check(op) \
    PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_LIST_SUBCLASS)
#define PyList_CheckExact(op) (Py_TYPE(op) == &PyList_Type)

PyAPI_FUNC(PyObject *) PyList_New(Py_ssize_t size);
PyAPI_FUNC(Py_ssize_t) PyList_Size(PyObject *);
PyAPI_FUNC(PyObject *) PyList_GetItem(PyObject *, Py_ssize_t);
PyAPI_FUNC(int) PyList_SetItem(PyObject *, Py_ssize_t, PyObject *);
PyAPI_FUNC(int) PyList_Insert(PyObject *, Py_ssize_t, PyObject *);
PyAPI_FUNC(int) PyList_Append(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyList_GetSlice(PyObject *, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(int) PyList_SetSlice(PyObject *, Py_ssize_t, Py_ssize_t, PyObject *);
PyAPI_FUNC(int) PyList_Sort(PyObject *);
PyAPI_FUNC(int) PyList_Reverse(PyObject *);
PyAPI_FUNC(PyObject *) PyList_AsTuple(PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyList_Extend(PyListObject *, PyObject *);

PyAPI_FUNC(int) PyList_ClearFreeList(void);
PyAPI_FUNC(void) _PyList_DebugMallocStats(FILE *out);
#endif

/* Macro, trading safety for speed */
#ifndef Py_LIMITED_API
#define PyList_GET_ITEM(op, i) (((PyListObject *)(op))->ob_item[i])
#define PyList_SET_ITEM(op, i, v) (((PyListObject *)(op))->ob_item[i] = (v))
#define PyList_GET_SIZE(op)    (assert(PyList_Check(op)),Py_SIZE(op))
#define _PyList_ITEMS(op)      (((PyListObject *)(op))->ob_item)
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_LISTOBJECT_H */
