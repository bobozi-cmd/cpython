/* List object implementation */

#include "Python.h"
#include "internal/pystate.h"
#include "accu.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>          /* For size_t */
#endif

/*[clinic input]
class list "PyListObject *" "&PyList_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f9b222678f9f71e0]*/

#include "clinic/listobject.c.h"

#ifdef Py_REF_DEBUG

static void trace_list_new(Py_ssize_t size, PyListObject *op);

static void trace_list_init(PyListObject *op);

static void set_item_hit(PyObject* obj);

static void append_item_hit(PyListObject* obj, PyObject* new_item);

static void trace_free_list(PyListObject *a);

#endif

/* Ensure ob_item has room for at least newsize elements, and set
 * ob_size to newsize.  If newsize > ob_size on entry, the content
 * of the new slots at exit is undefined heap trash; it's the caller's
 * responsibility to overwrite them with sane values.
 * The number of allocated elements may grow, shrink, or stay the same.
 * Failure is impossible if newsize <= self.allocated on entry, although
 * that partly relies on an assumption that the system realloc() never
 * fails when passed a number of bytes <= the number of bytes last
 * allocated (the C standard doesn't guarantee this, but it's hard to
 * imagine a realloc implementation where it wouldn't be true).
 * Note that self->ob_item may change, and even if newsize is less
 * than ob_size on entry.
 */

 /*确保ob_item至少有容纳newsize元素的空间,并将ob_size赋予newsize。
  *如果在输入newsize > obsize，则输出新的slots的内容则是未定义的堆；
  *调用者应该用新的元素覆盖掉他们。
  *已分配元素的数量可能会增加、减少或保持不变。
  *如果newsize <= self,则必定执行成功，即使这一部分依赖于一个假设，
  *当传递的字节数 <= 最后分配的字节数（C标准不能保证这一点，但很难想象到一个realloc实现是不能做到这一点）时，系统realloc（）不会失败
  *注意self->ob_item可能会改变，即使输入的newsize会小于ob_size
  */
static int
list_resize(PyListObject *self, Py_ssize_t newsize)
{
    PyObject **items;
    size_t new_allocated, num_allocated_bytes;
    Py_ssize_t allocated = self->allocated;

    /* Bypass realloc() when a previous overallocation is large enough
       to accommodate the newsize.  If the newsize falls lower than half
       the allocated size, then proceed with the realloc() to shrink the list.
    */
	/*当上一个过度分配大到足以容纳newsize时，绕过realloc()。
	 * 如果newsize低于所分配大小的一半，则继续执行realloc()来缩小列表。
	 */
    if (allocated >= newsize && newsize >= (allocated >> 1)) {
        assert(self->ob_item != NULL || newsize == 0);
        Py_SIZE(self) = newsize;  /* ((PyVarObject*)(self))->ob_size = newsize;  调整ob_size值 */
        return 0;
    }

    /* This over-allocates proportional to the list size, making room
     * for additional growth.  The over-allocation is mild, but is
     * enough to give linear-time amortized behavior over a long
     * sequence of appends() in the presence of a poorly-performing
     * system realloc().
     * The growth pattern is:  0, 4, 8, 16, 25, 35, 46, 58, 72, 88, ...
     * Note: new_allocated won't overflow because the largest possible value
     *       is PY_SSIZE_T_MAX * (9 / 8) + 6 which always fits in a size_t.
     */
	 /*这种过度分配与列表大小成比例，为额外的增长腾出空间。
	  *过度分配虽然温和，足以在存在性能较差的系统realloc()的情况下，
	  *对一长串appends()进行线性时间平摊行为
	  *增长模式为:0、4、8、16、25、35、46、58、72、88、……
	  *注意:new_allocated不会溢出，因为最大的可能值是PY_SSIZE_T_MAX *(9 / 8) + 6，它总是适合size_t。
	  */
    /* 计算重新申请的内存大小 */
	new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6);
    if (new_allocated > (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *)) {
        PyErr_NoMemory();
        return -1;
    }

    if (newsize == 0)
        new_allocated = 0;
	/* 拓展列表 */
    num_allocated_bytes = new_allocated * sizeof(PyObject *);
    /* 调用C中的realloc */
	items = (PyObject **)PyMem_Realloc(self->ob_item, num_allocated_bytes);
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->ob_item = items;
    Py_SIZE(self) = newsize;
    self->allocated = new_allocated;
    return 0;
}

/* Debug statistic to compare allocations with reuse through the free list */
#undef SHOW_ALLOC_COUNT
#ifdef SHOW_ALLOC_COUNT
static size_t count_alloc = 0;
static size_t count_reuse = 0;

static void
show_alloc(void)
{
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    if (!interp->core_config.show_alloc_count) {
        return;
    }

    fprintf(stderr, "List allocations: %" PY_FORMAT_SIZE_T "d\n",
        count_alloc);
    fprintf(stderr, "List reuse through freelist: %" PY_FORMAT_SIZE_T
        "d\n", count_reuse);
    fprintf(stderr, "%.2f%% reuse rate\n\n",
        (100.0*count_reuse/(count_alloc+count_reuse)));
}
#endif

/* Empty list reuse scheme to save calls to malloc and free */
/*清空列表重新使用模型以保存对 malloc 和 free 的调用*/
#ifndef PyList_MAXFREELIST
#define PyList_MAXFREELIST 80
#endif
static PyListObject *free_list[PyList_MAXFREELIST];
static int numfree = 0;

int
PyList_ClearFreeList(void)
{
    PyListObject *op;
    int ret = numfree;
    while (numfree) {
        op = free_list[--numfree];
        assert(PyList_CheckExact(op));
        PyObject_GC_Del(op);
    }
    return ret;
}

void
PyList_Fini(void)
{
    PyList_ClearFreeList();
}

/* Print summary info about the state of the optimized allocator */
/* 打印关于优化分配器状态的摘要信息 */
void
_PyList_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyListObject",
                           numfree, sizeof(PyListObject));
}

PyObject *
PyList_New(Py_ssize_t size)
{
    PyListObject *op;
#ifdef SHOW_ALLOC_COUNT
    static int initialized = 0;
    if (!initialized) {
        Py_AtExit(show_alloc);
        initialized = 1;
    }
#endif

    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
	// 为PyListObject对象申请空间
    if (numfree) {
		// 缓冲池可用
        numfree--;
        op = free_list[numfree];
        _Py_NewReference((PyObject *)op);
#ifdef SHOW_ALLOC_COUNT
        count_reuse++;
#endif
    } else {
		// 缓冲池不可用
        op = PyObject_GC_New(PyListObject, &PyList_Type);
        if (op == NULL)
            return NULL;
#ifdef SHOW_ALLOC_COUNT
        count_alloc++;
#endif
    }
	// 为PyListObject对象中维护的元素列表申请空间
    if (size <= 0)
        op->ob_item = NULL;
    else {
        op->ob_item = (PyObject **) PyMem_Calloc(size, sizeof(PyObject *));
        if (op->ob_item == NULL) {
            Py_DECREF(op);
            return PyErr_NoMemory();
        }
    }
    Py_SIZE(op) = size;     /* #define Py_SIZE(ob) (((PyVarObject*)(ob))->ob_size) */
    op->allocated = size;
    _PyObject_GC_TRACK(op);

#ifdef Py_REF_DEBUG
	trace_list_new(size, op);
#endif

    return (PyObject *) op;
}

Py_ssize_t
PyList_Size(PyObject *op)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    else
        return Py_SIZE(op);
}

static PyObject *indexerr = NULL;

PyObject *
PyList_GetItem(PyObject *op, Py_ssize_t i)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        if (indexerr == NULL) {
            indexerr = PyUnicode_FromString(
                "list index out of range");
            if (indexerr == NULL)
                return NULL;
        }
        PyErr_SetObject(PyExc_IndexError, indexerr);
        return NULL;
    }
    return ((PyListObject *)op) -> ob_item[i];
}

int
PyList_SetItem(PyObject *op, Py_ssize_t i,
               PyObject *newitem)
{
    PyObject **p;
	// 类型检查
    if (!PyList_Check(op)) {
        Py_XDECREF(newitem);
        PyErr_BadInternalCall();
        return -1;
    }
	// 索引检查
    if (i < 0 || i >= Py_SIZE(op)) {
        Py_XDECREF(newitem);
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        return -1;
    }
	// 设置元素
    p = ((PyListObject *)op) -> ob_item + i;
#ifdef Py_REF_DEBUG
	set_item_hit(newitem);
#endif
    Py_XSETREF(*p, newitem);
    return 0;
}

static int
ins1(PyListObject *self, Py_ssize_t where, PyObject *v)
{
    Py_ssize_t i, n = Py_SIZE(self);
    PyObject **items;
	// 插入为空
    if (v == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "cannot add more objects to list");
        return -1;
    }
	// 调整列表容量
    if (list_resize(self, n+1) < 0)
        return -1;

	// 确定插入点
    if (where < 0) {
		// list[-x]的逻辑
        where += n;
		// 逆索引+n以后依然小于0，则插入头部
        if (where < 0)
            where = 0;
    }
	// 插入的位数大于list长度，就直接插入尾部
    if (where > n)
        where = n;
	// 插入元素
    items = self->ob_item;
    for (i = n; --i >= where; )
        items[i+1] = items[i];
    Py_INCREF(v);
    items[where] = v;
    return 0;
}

int
PyList_Insert(PyObject *op, Py_ssize_t where, PyObject *newitem)
{
#ifdef Py_REF_DEBUG
	if(_Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		printf("PyList_Insert\n");
	}
#endif

    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return ins1((PyListObject *)op, where, newitem);
}

static int
app1(PyListObject *self, PyObject *v)
{
    Py_ssize_t n = PyList_GET_SIZE(self);

    assert (v != NULL);
    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "cannot add more objects to list");
        return -1;
    }

    if (list_resize(self, n+1) < 0)
        return -1;

    Py_INCREF(v);
    PyList_SET_ITEM(self, n, v);

#ifdef Py_REF_DEBUG
	append_item_hit(self, v);
#endif

    return 0;
}

int
PyList_Append(PyObject *op, PyObject *newitem)
{
    if (PyList_Check(op) && (newitem != NULL))
        return app1((PyListObject *)op, newitem);
    PyErr_BadInternalCall();
    return -1;
}

/* Methods */

static void
list_dealloc(PyListObject *op)
{
    Py_ssize_t i;
    PyObject_GC_UnTrack(op);
    Py_TRASHCAN_SAFE_BEGIN(op)
	// 销毁PyListObject对象维护的元素列表
    if (op->ob_item != NULL) {
		/* Do it backwards, for Christian Tismer.
		   There's a simple test case where somehow this reduces
		   thrashing when a *very* large list is created and
		   immediately deleted.
		   为克里斯蒂安·蒂斯默倒过来。有一个简单的测试用例，
		   当创建一个*非常*大的列表并立即删除时，它以某种方式减少了颠簸。
		   */
        i = Py_SIZE(op);
        while (--i >= 0) {
            Py_XDECREF(op->ob_item[i]);
        }
        PyMem_FREE(op->ob_item);
    }
	// 释放PyListObject自身，检查缓冲池是否满了，如果没满，则把该待删除的PyListObject对象放入缓冲池中
    if (numfree < PyList_MAXFREELIST && PyList_CheckExact(op))
        free_list[numfree++] = op;
    else
        Py_TYPE(op)->tp_free((PyObject *)op);
    Py_TRASHCAN_SAFE_END(op)
}

static PyObject *
list_repr(PyListObject *v)
{
    Py_ssize_t i;
    PyObject *s;
    _PyUnicodeWriter writer;

    if (Py_SIZE(v) == 0) {
        return PyUnicode_FromString("[]");
    }

    i = Py_ReprEnter((PyObject*)v);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("[...]") : NULL;
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    /* "[" + "1" + ", 2" * (len - 1) + "]" */
    writer.min_length = 1 + 1 + (2 + 1) * (Py_SIZE(v) - 1) + 1;

    if (_PyUnicodeWriter_WriteChar(&writer, '[') < 0)
        goto error;

	/* Do repr() on each element.  Note that this may mutate the list,
	   so must refetch the list size on each iteration.
	   对每个元素执行 repr（）。 注意这可能会改变列表，
	   因此，必须在每次迭代时重新获取list size*/
    for (i = 0; i < Py_SIZE(v); ++i) {
        if (i > 0) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0)
                goto error;
        }

        s = PyObject_Repr(v->ob_item[i]);
        if (s == NULL)
            goto error;

        if (_PyUnicodeWriter_WriteStr(&writer, s) < 0) {
            Py_DECREF(s);
            goto error;
        }
        Py_DECREF(s);
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, ']') < 0)
        goto error;

    Py_ReprLeave((PyObject *)v);
    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}

static Py_ssize_t
list_length(PyListObject *a)
{
#ifdef Py_REF_DEBUG
	trace_free_list(a);
#endif
    return Py_SIZE(a);
}

static int
list_contains(PyListObject *a, PyObject *el)
{
    PyObject *item;
    Py_ssize_t i;
    int cmp;

    for (i = 0, cmp = 0 ; cmp == 0 && i < Py_SIZE(a); ++i) {
        item = PyList_GET_ITEM(a, i);
        Py_INCREF(item);
        cmp = PyObject_RichCompareBool(el, item, Py_EQ);
        Py_DECREF(item);
    }
    return cmp;
}

static PyObject *
list_item(PyListObject *a, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        if (indexerr == NULL) {
            indexerr = PyUnicode_FromString(
                "list index out of range");
            if (indexerr == NULL)
                return NULL;
        }
        PyErr_SetObject(PyExc_IndexError, indexerr);
        return NULL;
    }
    Py_INCREF(a->ob_item[i]);
    return a->ob_item[i];
}

static PyObject *
list_slice(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    PyListObject *np;
    PyObject **src, **dest;
    Py_ssize_t i, len;
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    len = ihigh - ilow;
    np = (PyListObject *) PyList_New(len);
    if (np == NULL)
        return NULL;

    src = a->ob_item + ilow;
    dest = np->ob_item;
    for (i = 0; i < len; i++) {
        PyObject *v = src[i];
        Py_INCREF(v);
        dest[i] = v;
    }
    return (PyObject *)np;
}

PyObject *
PyList_GetSlice(PyObject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    if (!PyList_Check(a)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return list_slice((PyListObject *)a, ilow, ihigh);
}

static PyObject *
list_concat(PyListObject *a, PyObject *bb)
{
    Py_ssize_t size;
    Py_ssize_t i;
    PyObject **src, **dest;
    PyListObject *np;
    if (!PyList_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
                  "can only concatenate list (not \"%.200s\") to list",
                  bb->ob_type->tp_name);
        return NULL;
    }
#define b ((PyListObject *)bb)
    if (Py_SIZE(a) > PY_SSIZE_T_MAX - Py_SIZE(b))
        return PyErr_NoMemory();
    size = Py_SIZE(a) + Py_SIZE(b);
    np = (PyListObject *) PyList_New(size);
    if (np == NULL) {
        return NULL;
    }
    src = a->ob_item;
    dest = np->ob_item;
    for (i = 0; i < Py_SIZE(a); i++) {
        PyObject *v = src[i];
        Py_INCREF(v);
        dest[i] = v;
    }
    src = b->ob_item;
    dest = np->ob_item + Py_SIZE(a);
    for (i = 0; i < Py_SIZE(b); i++) {
        PyObject *v = src[i];
        Py_INCREF(v);
        dest[i] = v;
    }
    return (PyObject *)np;
#undef b
}

static PyObject *
list_repeat(PyListObject *a, Py_ssize_t n)
{
    Py_ssize_t i, j;
    Py_ssize_t size;
    PyListObject *np;
    PyObject **p, **items;
    PyObject *elem;
    if (n < 0)
        n = 0;
    if (n > 0 && Py_SIZE(a) > PY_SSIZE_T_MAX / n)
        return PyErr_NoMemory();
    size = Py_SIZE(a) * n;
    if (size == 0)
        return PyList_New(0);
    np = (PyListObject *) PyList_New(size);
    if (np == NULL)
        return NULL;

    items = np->ob_item;
    if (Py_SIZE(a) == 1) {
        elem = a->ob_item[0];
        for (i = 0; i < n; i++) {
            items[i] = elem;
            Py_INCREF(elem);
        }
        return (PyObject *) np;
    }
    p = np->ob_item;
    items = a->ob_item;
    for (i = 0; i < n; i++) {
        for (j = 0; j < Py_SIZE(a); j++) {
            *p = items[j];
            Py_INCREF(*p);
            p++;
        }
    }
    return (PyObject *) np;
}

static int
_list_clear(PyListObject *a)
{
    Py_ssize_t i;
    PyObject **item = a->ob_item;
    if (item != NULL) {
		/* Because XDECREF can recursively invoke operations on
		   this list, we make it empty first.
		   由于 XDECREF 可以递归调用此列表，因此我们首先将其设为空
		*/
        i = Py_SIZE(a);
        Py_SIZE(a) = 0;
        a->ob_item = NULL;
        a->allocated = 0;
        while (--i >= 0) {
            Py_XDECREF(item[i]);
        }
        PyMem_FREE(item);
    }
	/* Never fails; the return value can be ignored.
	   Note that there is no guarantee that the list is actually empty
	   at this point, because XDECREF may have populated it again!
	   不会失败;可以忽略返回值。
	   请注意，不能保证此时列表是空的
	   因为XDECREF可能已经再次填充了它！
	*/
    return 0;
}

/* a[ilow:ihigh] = v if v != NULL.
 * del a[ilow:ihigh] if v == NULL.
 *
 * Special speed gimmick:  when v is NULL and ihigh - ilow <= 8, it's
 * guaranteed the call cannot fail.
 */
static int
list_ass_slice(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
	/* Because [X]DECREF can recursively invoke list operations on
	   this list, we must postpone all [X]DECREF activity until
	   after the list is back in its canonical shape.  Therefore
	   we must allocate an additional array, 'recycle', into which
	   we temporarily copy the items that are deleted from the
	   list. :-(
	   因为 [X]DECREF 可以递归调用
	   此列表，我们必须推迟所有 [X]DECREF 活动，直到
	   在列表恢复其规范形状之后。 因此
	   我们必须分配一个额外的数组，"回收"，到其中
	   我们暂时复制从列表删除的项目。:-(*/
    PyObject *recycle_on_stack[8];
    PyObject **recycle = recycle_on_stack; /* will allocate more if needed */
    PyObject **item;
    PyObject **vitem = NULL;
    PyObject *v_as_SF = NULL; /* PySequence_Fast(v) */
    Py_ssize_t n; /* # of elements in replacement list */
    Py_ssize_t norig; /* # of elements in list getting replaced */
    Py_ssize_t d; /* Change in size */
    Py_ssize_t k;
    size_t s;
    int result = -1;            /* guilty until proved innocent */
#define b ((PyListObject *)v)
    if (v == NULL)
        n = 0;
    else {
        if (a == b) {
            /* Special case "a[i:j] = a" -- copy b first */
            v = list_slice(b, 0, Py_SIZE(b));
            if (v == NULL)
                return result;
            result = list_ass_slice(a, ilow, ihigh, v);
            Py_DECREF(v);
            return result;
        }
        v_as_SF = PySequence_Fast(v, "can only assign an iterable");
        if(v_as_SF == NULL)
            goto Error;
        n = PySequence_Fast_GET_SIZE(v_as_SF);
        vitem = PySequence_Fast_ITEMS(v_as_SF);
    }
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);

    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);

    norig = ihigh - ilow;
    assert(norig >= 0);
    d = n - norig;
    if (Py_SIZE(a) + d == 0) {
        Py_XDECREF(v_as_SF);
        return _list_clear(a);
    }
    item = a->ob_item;
	/* recycle the items that we are about to remove
	回收我们即将删除的项目*/
    s = norig * sizeof(PyObject *);
	/* If norig == 0, item might be NULL, in which case we may not memcpy from it.
	如果 norig == 0，则 item 可能为 NULL，在这种情况下，我们可能不会从中获取 memcpy。*/
    if (s) {
        if (s > sizeof(recycle_on_stack)) {
            recycle = (PyObject **)PyMem_MALLOC(s);
            if (recycle == NULL) {
                PyErr_NoMemory();
                goto Error;
            }
        }
        memcpy(recycle, &item[ilow], s);
    }

    if (d < 0) { /* Delete -d items */
        Py_ssize_t tail;
        tail = (Py_SIZE(a) - ihigh) * sizeof(PyObject *);
        memmove(&item[ihigh+d], &item[ihigh], tail);
        if (list_resize(a, Py_SIZE(a) + d) < 0) {
            memmove(&item[ihigh], &item[ihigh+d], tail);
            memcpy(&item[ilow], recycle, s);
            goto Error;
        }
        item = a->ob_item;
    }
    else if (d > 0) { /* Insert d items */
        k = Py_SIZE(a);
        if (list_resize(a, k+d) < 0)
            goto Error;
        item = a->ob_item;
        memmove(&item[ihigh+d], &item[ihigh],
            (k - ihigh)*sizeof(PyObject *));
    }
    for (k = 0; k < n; k++, ilow++) {
        PyObject *w = vitem[k];
        Py_XINCREF(w);
        item[ilow] = w;
    }
    for (k = norig - 1; k >= 0; --k)
        Py_XDECREF(recycle[k]);
    result = 0;
 Error:
    if (recycle != recycle_on_stack)
        PyMem_FREE(recycle);
    Py_XDECREF(v_as_SF);
    return result;
#undef b
}

int
PyList_SetSlice(PyObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    if (!PyList_Check(a)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return list_ass_slice((PyListObject *)a, ilow, ihigh, v);
}

static PyObject *
list_inplace_repeat(PyListObject *self, Py_ssize_t n)
{
    PyObject **items;
    Py_ssize_t size, i, j, p;


    size = PyList_GET_SIZE(self);
    if (size == 0 || n == 1) {
        Py_INCREF(self);
        return (PyObject *)self;
    }

    if (n < 1) {
        (void)_list_clear(self);
        Py_INCREF(self);
        return (PyObject *)self;
    }

    if (size > PY_SSIZE_T_MAX / n) {
        return PyErr_NoMemory();
    }

    if (list_resize(self, size*n) < 0)
        return NULL;

    p = size;
    items = self->ob_item;
    for (i = 1; i < n; i++) { /* Start counting at 1, not 0 */
        for (j = 0; j < size; j++) {
            PyObject *o = items[j];
            Py_INCREF(o);
            items[p++] = o;
        }
    }
    Py_INCREF(self);
    return (PyObject *)self;
}

static int
list_ass_item(PyListObject *a, Py_ssize_t i, PyObject *v)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        return -1;
    }
    if (v == NULL)
        return list_ass_slice(a, i, i+1, v);
    Py_INCREF(v);
    Py_SETREF(a->ob_item[i], v);
    return 0;
}

/*[clinic input]
list.insert

    index: Py_ssize_t
    object: object
    /

Insert object before index.
[clinic start generated code]*/

static PyObject *
list_insert_impl(PyListObject *self, Py_ssize_t index, PyObject *object)
/*[clinic end generated code: output=7f35e32f60c8cb78 input=858514cf894c7eab]*/
{
	if (_Py_GetMy_Debug() == 1) {
		printf("list_insert_impl\n");
	}
    if (ins1(self, index, object) == 0)
        Py_RETURN_NONE;
    return NULL;
}

/*[clinic input]
list.clear

Remove all items from list.
从列表中删除所有项目。
[clinic start generated code]*/

static PyObject *
list_clear_impl(PyListObject *self)
/*[clinic end generated code: output=67a1896c01f74362 input=ca3c1646856742f6]*/
{
    _list_clear(self);
    Py_RETURN_NONE;
}

/*[clinic input]
list.copy

Return a shallow copy of the list.
返回列表的浅副本。
[clinic start generated code]*/

static PyObject *
list_copy_impl(PyListObject *self)
/*[clinic end generated code: output=ec6b72d6209d418e input=6453ab159e84771f]*/
{
    return list_slice(self, 0, Py_SIZE(self));
}

/*[clinic input]
list.append

     object: object
     /

Append object to the end of the list.
将对象追加到列表的末尾。
[clinic start generated code]*/

static PyObject *
list_append(PyListObject *self, PyObject *object)
/*[clinic end generated code: output=7c096003a29c0eae input=43a3fe48a7066e91]*/
{
    if (app1(self, object) == 0)
        Py_RETURN_NONE;
    return NULL;
}

/*[clinic input]
list.extend

     iterable: object
     /

Extend list by appending elements from the iterable.
通过从可迭代对象中追加元素来扩展列表。
[clinic start generated code]*/

static PyObject *
list_extend(PyListObject *self, PyObject *iterable)
/*[clinic end generated code: output=630fb3bca0c8e789 input=9ec5ba3a81be3a4d]*/
{
    PyObject *it;      /* iter(v) */
    Py_ssize_t m;                  /* size of self */
    Py_ssize_t n;                  /* guess for size of iterable */
    Py_ssize_t mn;                 /* m + n */
    Py_ssize_t i;
    PyObject *(*iternext)(PyObject *);

	/* Special cases:
	   1) lists and tuples which can use PySequence_Fast ops
	   2) extending self to self requires making a copy first
	   特殊情况：
	   1）可以使用PySequence_Fast操作的列表和元组
	   2）将自我扩展到自我需要先制作副本
	*/
    if (PyList_CheckExact(iterable) || PyTuple_CheckExact(iterable) ||
                (PyObject *)self == iterable) {
        PyObject **src, **dest;
        iterable = PySequence_Fast(iterable, "argument must be iterable");
        if (!iterable)
            return NULL;
        n = PySequence_Fast_GET_SIZE(iterable);
        if (n == 0) {
            /* short circuit when iterable is empty */
            Py_DECREF(iterable);
            Py_RETURN_NONE;
        }
        m = Py_SIZE(self);
		/* It should not be possible to allocate a list large enough to cause
		an overflow on any relevant platform
		应该不可能分配足够大的列表以在任何相关平台上导致溢出*/
        assert(m < PY_SSIZE_T_MAX - n);
        if (list_resize(self, m + n) < 0) {
            Py_DECREF(iterable);
            return NULL;
        }
		/* note that we may still have self == iterable here for the
		 * situation a.extend(a), but the following code works
		 * in that case too.  Just make sure to resize self
		 * before calling PySequence_Fast_ITEMS.
		 * 请注意，我们可能仍然有 self == lierable的
		 * 情况 a.extend（a），但以下代码有效
		 * 在这种情况下也是如此。
		 * 只需确保在致电PySequence_Fast_ITEMS之前调整自我大小即可。
		 */
        /* populate the end of self with iterable's items */
        src = PySequence_Fast_ITEMS(iterable);
        dest = self->ob_item + m;
        for (i = 0; i < n; i++) {
            PyObject *o = src[i];
            Py_INCREF(o);
            dest[i] = o;
        }
        Py_DECREF(iterable);
        Py_RETURN_NONE;
    }

    it = PyObject_GetIter(iterable);
    if (it == NULL)
        return NULL;
    iternext = *it->ob_type->tp_iternext;

	/* Guess a result list size.
	 * 猜测结果列表大小。 */
    n = PyObject_LengthHint(iterable, 8);
    if (n < 0) {
        Py_DECREF(it);
        return NULL;
    }
    m = Py_SIZE(self);
    if (m > PY_SSIZE_T_MAX - n) {
		/* m + n overflowed; on the chance that n lied, and there really
		 * is enough room, ignore it.  If n was telling the truth, we'll
		 * eventually run out of memory during the loop.
		 * m + n 溢出;如果n撒谎，并且确实有足够的空间，请忽略它。
		 * 如果 n 说的是实话，我们最终会在循环期间耗尽内存。
		 */
    }
    else {
        mn = m + n;
        /* Make room. */
        if (list_resize(self, mn) < 0)
            goto error;
        /* Make the list sane again. */
        Py_SIZE(self) = m;
    }

    /* Run iterator to exhaustion. */
    for (;;) {
        PyObject *item = iternext(it);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_StopIteration))
                    PyErr_Clear();
                else
                    goto error;
            }
            break;
        }
        if (Py_SIZE(self) < self->allocated) {
            /* steals ref */
            PyList_SET_ITEM(self, Py_SIZE(self), item);
            ++Py_SIZE(self);
        }
        else {
            int status = app1(self, item);
            Py_DECREF(item);  /* append creates a new ref */
            if (status < 0)
                goto error;
        }
    }

    /* Cut back result list if initial guess was too large. */
    if (Py_SIZE(self) < self->allocated) {
        if (list_resize(self, Py_SIZE(self)) < 0)
            goto error;
    }

    Py_DECREF(it);
    Py_RETURN_NONE;

  error:
    Py_DECREF(it);
    return NULL;
}

PyObject *
_PyList_Extend(PyListObject *self, PyObject *iterable)
{
    return list_extend(self, iterable);
}

static PyObject *
list_inplace_concat(PyListObject *self, PyObject *other)
{
    PyObject *result;

    result = list_extend(self, other);
    if (result == NULL)
        return result;
    Py_DECREF(result);
    Py_INCREF(self);
    return (PyObject *)self;
}

/*[clinic input]
list.pop

    index: Py_ssize_t = -1
    /

Remove and return item at index (default last).

Raises IndexError if list is empty or index is out of range.
[clinic start generated code]*/

static PyObject *
list_pop_impl(PyListObject *self, Py_ssize_t index)
/*[clinic end generated code: output=6bd69dcb3f17eca8 input=b83675976f329e6f]*/
{
    PyObject *v;
    int status;

    if (Py_SIZE(self) == 0) {
        /* Special-case most common failure cause */
        PyErr_SetString(PyExc_IndexError, "pop from empty list");
        return NULL;
    }
    if (index < 0)
        index += Py_SIZE(self);
    if (index < 0 || index >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    v = self->ob_item[index];
    if (index == Py_SIZE(self) - 1) {
        status = list_resize(self, Py_SIZE(self) - 1);
        if (status >= 0)
            return v; /* and v now owns the reference the list had */
        else
            return NULL;
    }
    Py_INCREF(v);
    status = list_ass_slice(self, index, index+1, (PyObject *)NULL);
    if (status < 0) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
}

/* Reverse a slice of a list in place, from lo up to (exclusive) hi. */
static void
reverse_slice(PyObject **lo, PyObject **hi)
{
    assert(lo && hi);

    --hi;
    while (lo < hi) {
        PyObject *t = *lo;
        *lo = *hi;
        *hi = t;
        ++lo;
        --hi;
    }
}

/* Lots of code for an adaptive, stable, natural mergesort.  There are many
 * pieces to this algorithm; read listsort.txt for overviews and details.
 * 大量代码用于自适应、稳定、自然的合并排序。
 * 此算法有许多部分;阅读listsort.txt了解概述和详细信息。
 */

 /* A sortslice contains a pointer to an array of keys and a pointer to
  * an array of corresponding values.  In other words, keys[i]
  * corresponds with values[i].  If values == NULL, then the keys are
  * also the values.
  *
  * Several convenience routines are provided here, so that keys and
  * values are always moved in sync.
  *
  * 排序码包含指向keys数组的指针和指向相应values数组的指针。
  * 换句话说，keys[i]对应于values[i]。 如果values == NULL，则keys也是values。
  */


typedef struct {
    PyObject **keys;
    PyObject **values;
} sortslice;

Py_LOCAL_INLINE(void)
sortslice_copy(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j)
{
    s1->keys[i] = s2->keys[j];
    if (s1->values != NULL)
        s1->values[i] = s2->values[j];
}

Py_LOCAL_INLINE(void)
sortslice_copy_incr(sortslice *dst, sortslice *src)
{
    *dst->keys++ = *src->keys++;
    if (dst->values != NULL)
        *dst->values++ = *src->values++;
}

Py_LOCAL_INLINE(void)
sortslice_copy_decr(sortslice *dst, sortslice *src)
{
    *dst->keys-- = *src->keys--;
    if (dst->values != NULL)
        *dst->values-- = *src->values--;
}


Py_LOCAL_INLINE(void)
sortslice_memcpy(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j,
                 Py_ssize_t n)
{
    memcpy(&s1->keys[i], &s2->keys[j], sizeof(PyObject *) * n);
    if (s1->values != NULL)
        memcpy(&s1->values[i], &s2->values[j], sizeof(PyObject *) * n);
}

Py_LOCAL_INLINE(void)
sortslice_memmove(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j,
                  Py_ssize_t n)
{
    memmove(&s1->keys[i], &s2->keys[j], sizeof(PyObject *) * n);
    if (s1->values != NULL)
        memmove(&s1->values[i], &s2->values[j], sizeof(PyObject *) * n);
}

Py_LOCAL_INLINE(void)
sortslice_advance(sortslice *slice, Py_ssize_t n)
{
    slice->keys += n;
    if (slice->values != NULL)
        slice->values += n;
}

/* Comparison function: ms->key_compare, which is set at run-time in
 * listsort_impl to optimize for various special cases.
 * Returns -1 on error, 1 if x < y, 0 if x >= y.
 * 比较功能：ms->key_compare，在运行时设置listsort_impl，
 * 针对各种特殊情况进行优化。
 * 错误时返回-1，如果x<y返回1，x>=y返回0
 */

#define ISLT(X, Y) (*(ms->key_compare))(X, Y, ms)

 /* Compare X to Y via "<".  Goto "fail" if the comparison raises an
	error.  Else "k" is set to true if X<Y, and an "if (k)" block is
	started.  It makes more sense in context <wink>.  X and Y are PyObject*s.
	通过“<”比较X和Y。如果比较引发错误，则转到“失败”
	并且“k”被设置为真如果 X<Y，并且“if（k）”块被激活
	启动。它在内容<wink>中更有意义。X和Y是PyObject*s。
 */
#define IFLT(X, Y) if ((k = ISLT(X, Y)) < 0) goto fail;  \
           if (k)

 /* The maximum number of entries in a MergeState's pending-runs stack.
  * This is enough to sort arrays of size up to about
  *     32 * phi ** MAX_MERGE_PENDING
  * where phi ~= 1.618.  85 is ridiculouslylarge enough, good for an array
  * with 2**64 elements.
  *
  *MergeState 的挂起运行堆栈中的最大条目数。
  *这足以对大小约为
	 32 * phi ** MAX_MERGE_PENDING
  *的数组进行排序
  *其中 phi ~= 1.618。 85 已经足够，
  *非常适合具有 2**64 个元素的数组。
  */
#define MAX_MERGE_PENDING 85

  /* When we get into galloping mode, we stay there until both runs win less
   * often than MIN_GALLOP consecutive times.  See listsort.txt for more info.
   * 当我们进入高速模式时，我们会一直保持那种状态，
   * 直到两次运行成功的频率都低于连续MIN_GALLOP次。
   * 有关详细信息，请参阅listsort.txt。
   */
#define MIN_GALLOP 7

   /* Avoid malloc for small temp arrays. 避免malloc使用小型的临时数组*/
#define MERGESTATE_TEMP_SIZE 256

/* One MergeState exists on the stack per invocation of mergesort.  It's just
 * a convenient way to pass state around among the helper functions.
 * 每次调用 mergesort 时，堆栈上都存在一个 MergeState。
 * 这只是一个在帮助程序函数之间传递状态的便捷方法。
 */
struct s_slice {
    sortslice base;
    Py_ssize_t len;
};

typedef struct s_MergeState MergeState;
struct s_MergeState {
	/* This controls when we get *into* galloping mode.  It's initialized
	 * to MIN_GALLOP.  merge_lo and merge_hi tend to nudge it higher for
	 * random data, and lower for highly structured data.
	 * 这可以控制我们何时*int高速模式。
	 * 它被初始化为MIN_GALLOP。 对于随机数据，merge_lo和merge_hi往往会将其推高，
	 *对于高度结构化的数据，将其推得更低
	 */
    Py_ssize_t min_gallop;

	/* 'a' is temp storage to help with merges.  It contains room for
	 * alloced entries.
	 * "a"是临时存储，用于帮助合并。 它包含
	 * 分配条目的空间
	 */
    sortslice a;         /* may point to temparray below 可能指向下面的时间数组*/
    Py_ssize_t alloced;

	/* A stack of n pending runs yet to be merged.  Run #i starts at
	 * address base[i] and extends for len[i] elements.  It's always
	 * true (so long as the indices are in bounds) that
	 *
	 *     pending[i].base + pending[i].len == pending[i+1].base
	 *
	 * so we could cut the storage for this, but it's a minor amount,
	 * and keeping all the info explicit simplifies the code.
	 *
	 *一堆 n 个pends runs尚未合并。
	 *Run #i从base[i]的位置开始，并扩展到len[i]元素。
	 *这始终是正确的（只要索引在边界内）
	 *
	 *    pending[i].base + pending[i].len == pending[i+1].base
	 *
	 *因此，我们可以为此削减存储空间，但这是一个很小的数量，
	 *并且明确保留所有信息可以简化代码。
	 */
    int n;
    struct s_slice pending[MAX_MERGE_PENDING];

    /* 'a' points to this when possible, rather than muck with malloc. */
    PyObject *temparray[MERGESTATE_TEMP_SIZE];

	/* This is the function we will use to compare two keys,
	 * even when none of our special cases apply and we have to use
	 * safe_object_compare.
	 * 这是我们将用于比较两个keys的函数，
	 * 即使我们的特殊情况都不适用，
	 * 并且我们必须使用safe_object_compare。*/
    int (*key_compare)(PyObject *, PyObject *, MergeState *);

	/* This function is used by unsafe_object_compare to optimize comparisons
	 * when we know our list is type-homogeneous but we can't assume anything else.
	 * In the pre-sort check it is set equal to key->ob_type->tp_richcompare
	 * unsafe_object_compare使用此函数来优化对比，
	 * 当我们知道我们的列表是类型同构的，但我们不能假设任何其他内容时。
	 * 在预排序检查中，它设置为等于key>ob_type>tp_richcompare*/
    PyObject *(*key_richcompare)(PyObject *, PyObject *, int);

	/* This function is used by unsafe_tuple_compare to compare the first elements
	 * of tuples. It may be set to safe_object_compare, but the idea is that hopefully
	 * we can assume more, and use one of the special-case compares.
	 * unsafe_tuple_compare使用此函数来与第一个元素的元组进行对比
	 * 它可能被设置为safe_object_compare，但这个想法是希望
	 * 我们可以假设更多，并使用其中一个特殊情况比较。*/
    int (*tuple_elem_compare)(PyObject *, PyObject *, MergeState *);
};

/* binarysort is the best method for sorting small arrays: it does
   few compares, but can do data movement quadratic in the number of
   elements.
   [lo, hi) is a contiguous slice of a list, and is sorted via
   binary insertion.  This sort is stable.
   On entry, must have lo <= start <= hi, and that [lo, start) is already
   sorted (pass start == lo if you don't know!).
   If islt() complains return -1, else 0.
   Even in case of error, the output slice will be some permutation of
   the input (nothing is lost or duplicated).
   二进制排序是排序小数组的最佳方法：
   它很少进行比较，但可以做数据的二次移动在
   元素的数量上。
   [lo， hi） 是列表的连续切片，排序方式为
   二进制插入。 这种排序是稳定的。
   输入方面，必须具有 lo <= start <= hi，并且 [lo， start） 已经存在
   排序（如果你不知道，通过开始==lo！
   如果 islt（） 抱怨返回 -1，否则为 0。
   即使在出现错误的情况下，输出切片也将是
   输入（不会丢失或重复任何内容）。
*/
static int
binarysort(MergeState *ms, sortslice lo, PyObject **hi, PyObject **start)
{
    Py_ssize_t k;
    PyObject **l, **p, **r;
    PyObject *pivot;

    assert(lo.keys <= start && start <= hi);
    /* assert [lo, start) is sorted */
    if (lo.keys == start)
        ++start;
    for (; start < hi; ++start) {
        /* set l to where *start belongs */
        l = lo.keys;
        r = start;
        pivot = *r;
        /* Invariants:
         * pivot >= all in [lo, l).
         * pivot  < all in [r, start).
         * The second is vacuously true at the start.
         */
        assert(l < r);
        do {
            p = l + ((r - l) >> 1);
            IFLT(pivot, *p)
                r = p;
            else
                l = p+1;
        } while (l < r);
        assert(l == r);
		/* The invariants still hold, so pivot >= all in [lo, l) and
		   pivot < all in [l, start), so pivot belongs at l.  Note
		   that if there are elements equal to pivot, l points to the
		   first slot after them -- that's why this sort is stable.
		   Slide over to make room.
		   Caution: using memmove is much slower under MSVC 5;
		   we're not usually moving many slots.
		   不变量仍然成立，因此pivot> = all in [lo， l）b并且
		   pivot<all in[l， start） ，因此pivot属于 l。 注意
		   如果存在等于pivot的元素，则 l 指向
		   在他们之后的第一个slot -- 这就是为什么这种类型是稳定的。
		   跳过来腾出空间。
		   注意：在MSVC 5下使用memmove要慢得多;
		   我们通常不会移动很多plots。*/
        for (p = start; p > l; --p)
            *p = *(p-1);
        *l = pivot;
        if (lo.values != NULL) {
            Py_ssize_t offset = lo.values - lo.keys;
            p = start + offset;
            pivot = *p;
            l += offset;
            for (p = start + offset; p > l; --p)
                *p = *(p-1);
            *l = pivot;
        }
    }
    return 0;

 fail:
    return -1;
}

/*
Return the length of the run beginning at lo, in the slice [lo, hi).  lo < hi
is required on entry.  "A run" is the longest ascending sequence, with

	lo[0] <= lo[1] <= lo[2] <= ...

or the longest descending sequence, with

	lo[0] > lo[1] > lo[2] > ...

Boolean *descending is set to 0 in the former case, or to 1 in the latter.
For its intended use in a stable mergesort, the strictness of the defn of
"descending" is needed so that the caller can safely reverse a descending
sequence without violating stability (strict > ensures there are no equal
elements to get out of order).

Returns -1 in case of error.
返回从 lo 开始的运行长度，在切片 [lo， hi] 中。
在输入时需要lo < hi。 "A run"是最长的升序序列，具有

lo[0] <= lo[1] <= lo[2] <= ...

或最长的降序序列，使用

lo[0] > lo[1] > lo[2] > ...

在前一种情况下，布尔值 *降序设置为 0，在后者中设置为 1。
对于其在稳定合并排序中的预期用途，其严格
需要"降序"，以便调用方可以安全地反转降序
序列不违反稳定性（严格>确保没有相等性
元素来无序）。

如果错误返回-1
*/
static Py_ssize_t
count_run(MergeState *ms, PyObject **lo, PyObject **hi, int *descending)
{
    Py_ssize_t k;
    Py_ssize_t n;

    assert(lo < hi);
    *descending = 0;
    ++lo;
    if (lo == hi)
        return 1;

    n = 2;
    IFLT(*lo, *(lo-1)) {
        *descending = 1;
        for (lo = lo+1; lo < hi; ++lo, ++n) {
            IFLT(*lo, *(lo-1))
                ;
            else
                break;
        }
    }
    else {
        for (lo = lo+1; lo < hi; ++lo, ++n) {
            IFLT(*lo, *(lo-1))
                break;
        }
    }

    return n;
fail:
    return -1;
}

/*
Locate the proper position of key in a sorted vector; if the vector contains
an element equal to key, return the position immediately to the left of
the leftmost equal element.  [gallop_right() does the same except returns
the position to the right of the rightmost equal element (if any).]

在排序的向量中定位key的正确位置;如果向量包含
一个等于key的元素，返回位置立即位于
最左边的相等元素。 [gallop_right（） 执行相同的操作，但返回除外
最右边的相等元素（如果有）右侧的位置。

"a" is a sorted vector with n elements, starting at a[0].  n must be > 0.

"a" 是具有 n 个元素的排序向量，从 a[0] 开始。 n 必须> 0

"hint" is an index at which to begin the search, 0 <= hint < n.  The closer
hint is to the final result, the faster this runs.

"hint"是开始搜索的索引，0 <= hit < n。 越近
hit是最终结果，运行速度越快。

The return value is the int k in 0..n such that

	a[k-1] < key <= a[k]

pretending that *(a-1) is minus infinity and a[n] is plus infinity.  IOW,
key belongs at index k; or, IOW, the first k elements of a should precede
key, and the last n-k should follow key.

返回值是 0..n 中的 int k，使得
a[k-1] < 键 <= a[k]
假如 *（a-1） 是负无穷大，a[n] 是正无穷大。 IOW
key属于索引 k;或者，IOW，a 的前 k 个元素应该在key前面，
最后一个 n-k 应跟在key后面。

Returns -1 on error.  See listsort.txt for info on the method.

出错时返回 -1。 有关该方法的信息，请参阅listsort.txt。
*/
static Py_ssize_t
gallop_left(MergeState *ms, PyObject *key, PyObject **a, Py_ssize_t n, Py_ssize_t hint)
{
    Py_ssize_t ofs;
    Py_ssize_t lastofs;
    Py_ssize_t k;

    assert(key && a && n > 0 && hint >= 0 && hint < n);

    a += hint;
    lastofs = 0;
    ofs = 1;
    IFLT(*a, key) {
        /* a[hint] < key -- gallop right, until
         * a[hint + lastofs] < key <= a[hint + ofs]
         */
        const Py_ssize_t maxofs = n - hint;             /* &a[n-1] is highest */
        while (ofs < maxofs) {
            IFLT(a[ofs], key) {
                lastofs = ofs;
                assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
                ofs = (ofs << 1) + 1;
            }
            else                /* key <= a[hint + ofs] */
                break;
        }
        if (ofs > maxofs)
            ofs = maxofs;
		/* Translate back to offsets relative to &a[0].
		 *转换回相对于 &a[0] 的正偏移量。*/
        lastofs += hint;
        ofs += hint;
    }
    else {
        /* key <= a[hint] -- gallop left, until
         * a[hint - ofs] < key <= a[hint - lastofs]
         */
        const Py_ssize_t maxofs = hint + 1;             /* &a[0] is lowest */
        while (ofs < maxofs) {
            IFLT(*(a-ofs), key)
                break;
            /* key <= a[hint - ofs] */
            lastofs = ofs;
            assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
            ofs = (ofs << 1) + 1;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to positive offsets relative to &a[0]. */
        k = lastofs;
        lastofs = hint - ofs;
        ofs = hint - k;
    }
    a -= hint;

    assert(-1 <= lastofs && lastofs < ofs && ofs <= n);
    /* Now a[lastofs] < key <= a[ofs], so key belongs somewhere to the
     * right of lastofs but no farther right than ofs.  Do a binary
     * search, with invariant a[lastofs-1] < key <= a[ofs].
     */
    ++lastofs;
    while (lastofs < ofs) {
        Py_ssize_t m = lastofs + ((ofs - lastofs) >> 1);

        IFLT(a[m], key)
            lastofs = m+1;              /* a[m] < key */
        else
            ofs = m;                    /* key <= a[m] */
    }
    assert(lastofs == ofs);             /* so a[ofs-1] < key <= a[ofs] */
    return ofs;

fail:
    return -1;
}

/*
Exactly like gallop_left(), except that if key already exists in a[0:n],
finds the position immediately to the right of the rightmost equal value.

与 gallop_left（） 完全一样，只是如果key已经存在于 a[0：n] 中，
则立即找到最右边等值的位置。

The return value is the int k in 0..n such that

	a[k-1] <= key < a[k]

or -1 if error.

返回值是 0..n 中的 int k，使得

a[k-1] <= key < a[k]

如果出现错误，则为 -1。

The code duplication is massive, but this is enough different given that
we're sticking to "<" comparisons that it's much harder to follow if
written as one routine with yet another "left or right?" flag.
有大规模的代码重复，但考虑到
我们坚持"<"的比较，这会更加困难如果
写成一个例程，还有另一个"左或右？"标志。
*/
static Py_ssize_t
gallop_right(MergeState *ms, PyObject *key, PyObject **a, Py_ssize_t n, Py_ssize_t hint)
{
    Py_ssize_t ofs;
    Py_ssize_t lastofs;
    Py_ssize_t k;

    assert(key && a && n > 0 && hint >= 0 && hint < n);

    a += hint;
    lastofs = 0;
    ofs = 1;
    IFLT(key, *a) {
        /* key < a[hint] -- gallop left, until
         * a[hint - ofs] <= key < a[hint - lastofs]
         */
        const Py_ssize_t maxofs = hint + 1;             /* &a[0] is lowest */
        while (ofs < maxofs) {
            IFLT(key, *(a-ofs)) {
                lastofs = ofs;
                assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
                ofs = (ofs << 1) + 1;
            }
            else                /* a[hint - ofs] <= key */
                break;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to positive offsets relative to &a[0]. */
        k = lastofs;
        lastofs = hint - ofs;
        ofs = hint - k;
    }
    else {
        /* a[hint] <= key -- gallop right, until
         * a[hint + lastofs] <= key < a[hint + ofs]
        */
        const Py_ssize_t maxofs = n - hint;             /* &a[n-1] is highest */
        while (ofs < maxofs) {
            IFLT(key, a[ofs])
                break;
            /* a[hint + ofs] <= key */
            lastofs = ofs;
            assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
            ofs = (ofs << 1) + 1;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to offsets relative to &a[0]. */
        lastofs += hint;
        ofs += hint;
    }
    a -= hint;

    assert(-1 <= lastofs && lastofs < ofs && ofs <= n);
	/* Now a[lastofs] <= key < a[ofs], so key belongs somewhere to the
	 * right of lastofs but no farther right than ofs.  Do a binary
	 * search, with invariant a[lastofs-1] <= key < a[ofs].
	 *现在 a[lastofs] <= key < a[ofs]，所以 key 属于
	 *lastofs 右边的某个地方，但比 ofs 更靠右边。
	 *执行二进制搜索，使用不变的 a[lastofs-1] <= key < a[ofs]。
	 */
    ++lastofs;
    while (lastofs < ofs) {
        Py_ssize_t m = lastofs + ((ofs - lastofs) >> 1);

        IFLT(key, a[m])
            ofs = m;                    /* key < a[m] */
        else
            lastofs = m+1;              /* a[m] <= key */
    }
    assert(lastofs == ofs);             /* so a[ofs-1] <= key < a[ofs] */
    return ofs;

fail:
    return -1;
}

/* Conceptually a MergeState's constructor.
 *从概念上讲，MergeState的构造函数。*/
static void
merge_init(MergeState *ms, Py_ssize_t list_size, int has_keyfunc)
{
    assert(ms != NULL);
    if (has_keyfunc) {
		/* The temporary space for merging will need at most half the list
		 * size rounded up.  Use the minimum possible space so we can use the
		 * rest of temparray for other things.  In particular, if there is
		 * enough extra space, listsort() will use it to store the keys.
		 *用于合并的临时空间四舍五入最多需要列表大小的一半。
		 *使用尽可能小的空间，
		 *这样我们就可以将 temparray 的其余部分用于其他事情。
		 *特别是，如果有足够的额外空间，listsort（） 将使用它来存储密钥。
		 */
        ms->alloced = (list_size + 1) / 2;

        /* ms->alloced describes how many keys will be stored at
           ms->temparray, but we also need to store the values.  Hence,
           ms->alloced is capped at half of MERGESTATE_TEMP_SIZE. */
        if (MERGESTATE_TEMP_SIZE / 2 < ms->alloced)
            ms->alloced = MERGESTATE_TEMP_SIZE / 2;
        ms->a.values = &ms->temparray[ms->alloced];
    }
    else {
        ms->alloced = MERGESTATE_TEMP_SIZE;
        ms->a.values = NULL;
    }
    ms->a.keys = ms->temparray;
    ms->n = 0;
    ms->min_gallop = MIN_GALLOP;
}

/* Free all the temp memory owned by the MergeState.  This must be called
 * when you're done with a MergeState, and may be called before then if
 * you want to free the temp memory early.
 * 释放 MergeState 拥有的所有临时内存。 当您完成 MergeState 时，必须调用此项，
 * 如果要提前释放临时内存，则可以在此之前调用它
 */
static void
merge_freemem(MergeState *ms)
{
    assert(ms != NULL);
    if (ms->a.keys != ms->temparray)
        PyMem_Free(ms->a.keys);
}


/* Ensure enough temp memory for 'need' array slots is available.
 * Returns 0 on success and -1 if the memory can't be gotten.
 * 确保有足够的临时内存供“需要的”插槽数列使用
 * 成功时返回 0，如果无法获得内存，则返回 -1。
 */
static int
merge_getmem(MergeState *ms, Py_ssize_t need)
{
    int multiplier;

    assert(ms != NULL);
    if (need <= ms->alloced)
        return 0;

    multiplier = ms->a.values != NULL ? 2 : 1;

    /* Don't realloc!  That can cost cycles to copy the old data, but
     * we don't care what's in the block.
     */
    merge_freemem(ms);
    if ((size_t)need > PY_SSIZE_T_MAX / sizeof(PyObject *) / multiplier) {
        PyErr_NoMemory();
        return -1;
    }
    ms->a.keys = (PyObject **)PyMem_Malloc(multiplier * need
                                          * sizeof(PyObject *));
    if (ms->a.keys != NULL) {
        ms->alloced = need;
        if (ms->a.values != NULL)
            ms->a.values = &ms->a.keys[need];
        return 0;
    }
    PyErr_NoMemory();
    return -1;
}
#define MERGE_GETMEM(MS, NEED) ((NEED) <= (MS)->alloced ? 0 :   \
                                merge_getmem(MS, NEED))

/* Merge the na elements starting at ssa with the nb elements starting at
 * ssb.keys = ssa.keys + na in a stable way, in-place.  na and nb must be > 0.
 * Must also have that ssa.keys[na-1] belongs at the end of the merge, and
 * should have na <= nb.  See listsort.txt for more info.  Return 0 if
 * successful, -1 if error.
 */
 /*
 * 顺着（从第一位到最后一位）合并,合并到ssa里面
 * na和nb必须大于0。还必须具有ssa[na-1]属于合并的末尾，并且应具有na<=nb。
 * 如果成功，则返回0；如果错误，则返回-1。
 */
static Py_ssize_t
merge_lo(MergeState *ms, sortslice ssa, Py_ssize_t na,
         sortslice ssb, Py_ssize_t nb)
{
    Py_ssize_t k;
    sortslice dest;
    int result = -1;            /* guilty until proved innocent */
    Py_ssize_t min_gallop;

    assert(ms && ssa.keys && ssb.keys && na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);
    if (MERGE_GETMEM(ms, na) < 0)
        return -1;
    sortslice_memcpy(&ms->a, 0, &ssa, 0, na);
    dest = ssa;
    ssa = ms->a;

    sortslice_copy_incr(&dest, &ssb);
    --nb;
    if (nb == 0)
        goto Succeed;
    if (na == 1)
        goto CopyB;

    min_gallop = ms->min_gallop;
    for (;;) {
        Py_ssize_t acount = 0;          /* # of times A won in a row */
        Py_ssize_t bcount = 0;          /* # of times B won in a row */

        /* Do the straightforward thing until (if ever) one run
         * appears to win consistently.
         */
        for (;;) {
            assert(na > 1 && nb > 0);
            k = ISLT(ssb.keys[0], ssa.keys[0]);
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_copy_incr(&dest, &ssb);
                ++bcount;
                acount = 0;
                --nb;
                if (nb == 0)
                    goto Succeed;
                if (bcount >= min_gallop)
                    break;
            }
            else {
                sortslice_copy_incr(&dest, &ssa);
                ++acount;
                bcount = 0;
                --na;
                if (na == 1)
                    goto CopyB;
                if (acount >= min_gallop)
                    break;
            }
        }

        /* One run is winning so consistently that galloping may
         * be a huge win.  So try that, and continue galloping until
         * (if ever) neither run appears to be winning consistently
         * anymore.
         */
        ++min_gallop;
        do {
            assert(na > 1 && nb > 0);
            min_gallop -= min_gallop > 1;
            ms->min_gallop = min_gallop;
            k = gallop_right(ms, ssb.keys[0], ssa.keys, na, 0);
            acount = k;
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_memcpy(&dest, 0, &ssa, 0, k);
                sortslice_advance(&dest, k);
                sortslice_advance(&ssa, k);
                na -= k;
                if (na == 1)
                    goto CopyB;
                /* na==0 is impossible now if the comparison
                 * function is consistent, but we can't assume
                 * that it is.
                 */
                if (na == 0)
                    goto Succeed;
            }
            sortslice_copy_incr(&dest, &ssb);
            --nb;
            if (nb == 0)
                goto Succeed;

            k = gallop_left(ms, ssa.keys[0], ssb.keys, nb, 0);
            bcount = k;
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_memmove(&dest, 0, &ssb, 0, k);
                sortslice_advance(&dest, k);
                sortslice_advance(&ssb, k);
                nb -= k;
                if (nb == 0)
                    goto Succeed;
            }
            sortslice_copy_incr(&dest, &ssa);
            --na;
            if (na == 1)
                goto CopyB;
        } while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
        ++min_gallop;           /* penalize it for leaving galloping mode */
        ms->min_gallop = min_gallop;
    }
Succeed:
    result = 0;
Fail:
    if (na)
        sortslice_memcpy(&dest, 0, &ssa, 0, na);
    return result;
CopyB:
    assert(na == 1 && nb > 0);
    /* The last element of ssa belongs at the end of the merge. */
    sortslice_memmove(&dest, 0, &ssb, 0, nb);
    sortslice_copy(&dest, nb, &ssa, 0);
    return 0;
}


/* Merge the na elements starting at pa with the nb elements starting at
 * ssb.keys = ssa.keys + na in a stable way, in-place.  na and nb must be > 0.
 * Must also have that ssa.keys[na-1] belongs at the end of the merge, and
 * should have na >= nb.  See listsort.txt for more info.  Return 0 if
 * successful, -1 if error.
 */
 /*
 * 倒着合并（从最后一位到第一位），合并到ssb里面
 * na和nb必须大于0。还必须具有ssa[na-1]属于合并的末尾，并且应具有na>=nb。
 * 如果成功，则返回0；如果错误，则返回-1。
 */
static Py_ssize_t
merge_hi(MergeState *ms, sortslice ssa, Py_ssize_t na,
         sortslice ssb, Py_ssize_t nb)
{
    Py_ssize_t k;
    sortslice dest, basea, baseb;
    int result = -1;            /* guilty until proved innocent */
    Py_ssize_t min_gallop;

    assert(ms && ssa.keys && ssb.keys && na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);
    if (MERGE_GETMEM(ms, nb) < 0)
        return -1;
    dest = ssb;
    sortslice_advance(&dest, nb-1);
    sortslice_memcpy(&ms->a, 0, &ssb, 0, nb);
    basea = ssa;
    baseb = ms->a;
    ssb.keys = ms->a.keys + nb - 1;
    if (ssb.values != NULL)
        ssb.values = ms->a.values + nb - 1;
    sortslice_advance(&ssa, na - 1);

    sortslice_copy_decr(&dest, &ssa);
    --na;
    if (na == 0)
        goto Succeed;
    if (nb == 1)
        goto CopyA;

    min_gallop = ms->min_gallop;
    for (;;) {
        Py_ssize_t acount = 0;          /* # of times A won in a row */
        Py_ssize_t bcount = 0;          /* # of times B won in a row */

        /* Do the straightforward thing until (if ever) one run
         * appears to win consistently.
         */
        for (;;) {
            assert(na > 0 && nb > 1);
            k = ISLT(ssb.keys[0], ssa.keys[0]);
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_copy_decr(&dest, &ssa);
                ++acount;
                bcount = 0;
                --na;
                if (na == 0)
                    goto Succeed;
                if (acount >= min_gallop)
                    break;
            }
            else {
                sortslice_copy_decr(&dest, &ssb);
                ++bcount;
                acount = 0;
                --nb;
                if (nb == 1)
                    goto CopyA;
                if (bcount >= min_gallop)
                    break;
            }
        }

        /* One run is winning so consistently that galloping may
         * be a huge win.  So try that, and continue galloping until
         * (if ever) neither run appears to be winning consistently
         * anymore.
         */
        ++min_gallop;
        do {
            assert(na > 0 && nb > 1);
            min_gallop -= min_gallop > 1;
            ms->min_gallop = min_gallop;
            k = gallop_right(ms, ssb.keys[0], basea.keys, na, na-1);
            if (k < 0)
                goto Fail;
            k = na - k;
            acount = k;
            if (k) {
                sortslice_advance(&dest, -k);
                sortslice_advance(&ssa, -k);
                sortslice_memmove(&dest, 1, &ssa, 1, k);
                na -= k;
                if (na == 0)
                    goto Succeed;
            }
            sortslice_copy_decr(&dest, &ssb);
            --nb;
            if (nb == 1)
                goto CopyA;

            k = gallop_left(ms, ssa.keys[0], baseb.keys, nb, nb-1);
            if (k < 0)
                goto Fail;
            k = nb - k;
            bcount = k;
            if (k) {
                sortslice_advance(&dest, -k);
                sortslice_advance(&ssb, -k);
                sortslice_memcpy(&dest, 1, &ssb, 1, k);
                nb -= k;
                if (nb == 1)
                    goto CopyA;
                /* nb==0 is impossible now if the comparison
                 * function is consistent, but we can't assume
                 * that it is.
                 */
                if (nb == 0)
                    goto Succeed;
            }
            sortslice_copy_decr(&dest, &ssa);
            --na;
            if (na == 0)
                goto Succeed;
        } while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
        ++min_gallop;           /* penalize it for leaving galloping mode */
        ms->min_gallop = min_gallop;
    }
Succeed:
    result = 0;
Fail:
    if (nb)
        sortslice_memcpy(&dest, -(nb-1), &baseb, 0, nb);
    return result;
CopyA:
    assert(nb == 1 && na > 0);
    /* The first element of ssb belongs at the front of the merge. */
    sortslice_memmove(&dest, 1-na, &ssa, 1-na, na);
    sortslice_advance(&dest, -na);
    sortslice_advance(&ssa, -na);
    sortslice_copy(&dest, 0, &ssb, 0);
    return 0;
}

/* Merge the two runs at stack indices i and i+1.
 * Returns 0 on success, -1 on error.
 */
 /*
  * 合并堆栈索引i和i+1处的2个run。
  * 成功时返回0，错误时返回-1。
 */
static Py_ssize_t
merge_at(MergeState *ms, Py_ssize_t i)
{
    sortslice ssa, ssb;
    Py_ssize_t na, nb;
    Py_ssize_t k;

    assert(ms != NULL);
    assert(ms->n >= 2);
    assert(i >= 0);
    assert(i == ms->n - 2 || i == ms->n - 3);

    ssa = ms->pending[i].base;
    na = ms->pending[i].len;
    ssb = ms->pending[i+1].base;
    nb = ms->pending[i+1].len;
    assert(na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);

	/* Record the length of the combined runs; if i is the 3rd-last
	 * run now, also slide over the last run (which isn't involved
	 * in this merge).  The current run i+1 goes away in any case.
	 */
	 /*
	  * 记录组合运行的长度；如果我现在是第三次最后一次跑步，
	  * 也可以滑动到最后一次跑步（这与此合并无关）。
	  * 当前运行i+1在任何情况下都会消失。
	 */
    ms->pending[i].len = na + nb;
    if (i == ms->n - 3)
        ms->pending[i+1] = ms->pending[i+2];
    --ms->n;

    /* Where does b start in a?  Elements in a before that can be
     * ignored (already in place).
     */
    k = gallop_right(ms, *ssb.keys, ssa.keys, na, 0);
    if (k < 0)
        return -1;
    sortslice_advance(&ssa, k);
    na -= k;
    if (na == 0)
        return 0;

    /* Where does a end in b?  Elements in b after that can be
     * ignored (already in place).
     */
    nb = gallop_left(ms, ssa.keys[na-1], ssb.keys, nb, nb-1);
    if (nb <= 0)
        return nb;

    /* Merge what remains of the runs, using a temp array with
     * min(na, nb) elements.
     */
    if (na <= nb)
        return merge_lo(ms, ssa, na, ssb, nb);
    else
        return merge_hi(ms, ssa, na, ssb, nb);
}

/* Examine the stack of runs waiting to be merged, merging adjacent runs
 * until the stack invariants are re-established:
 *
 * 1. len[-3] > len[-2] + len[-1]
 * 2. len[-2] > len[-1]
 *
 * See listsort.txt for more info.
 *
 * Returns 0 on success, -1 on error.
 */
 /*
  * 检查等待合并的管路堆栈，合并相邻管路，直到重新建立堆栈不变量：
  * 1. len[-3] > len[-2] + len[-1]
  * 2. len[-2] > len[-1]
  * 成功时返回0，错误时返回-1
  */
static int
merge_collapse(MergeState *ms)
{
    struct s_slice *p = ms->pending;

    assert(ms);
    while (ms->n > 1) {
        Py_ssize_t n = ms->n - 2;
        if ((n > 0 && p[n-1].len <= p[n].len + p[n+1].len) ||
            (n > 1 && p[n-2].len <= p[n-1].len + p[n].len)) {
            if (p[n-1].len < p[n+1].len)
                --n;
            if (merge_at(ms, n) < 0)
                return -1;
        }
        else if (p[n].len <= p[n+1].len) {
            if (merge_at(ms, n) < 0)
                return -1;
        }
        else
            break;
    }
    return 0;
}

/* Regardless of invariants, merge all runs on the stack until only one
 * remains.  This is used at the end of the mergesort.
 *
 * Returns 0 on success, -1 on error.
 */
 /*
  * 不管不变量如何，合并堆栈上的所有运行，直到只剩下一个。这在合并排序的末尾使用。
  * 成功时返回0，错误时返回-1。
 */
static int
merge_force_collapse(MergeState *ms)
{
    struct s_slice *p = ms->pending;

    assert(ms);
    while (ms->n > 1) {
        Py_ssize_t n = ms->n - 2;
        if (n > 0 && p[n-1].len < p[n+1].len)
            --n;
        if (merge_at(ms, n) < 0)
            return -1;
    }
    return 0;
}

/* Compute a good value for the minimum run length; natural runs shorter
 * than this are boosted artificially via binary insertion.
 *
 * If n < 64, return n (it's too small to bother with fancy stuff).
 * Else if n is an exact power of 2, return 32.
 * Else return an int k, 32 <= k <= 64, such that n/k is close to, but
 * strictly less than, an exact power of 2.
 *
 * See listsort.txt for more info.
 */
 /*
  * 计算最小运行长度的良好值；短于此的自然运行通过二进制插入人为地增强。
  * 如果n<64，则返回n（它太小了，无法处理复杂的东西）。
  * 如果n是2的精确幂，则返回32。否则返回int k，32<=k<=64，因此n/k接近但严格小于2的精确幂。
 */
static Py_ssize_t
merge_compute_minrun(Py_ssize_t n)
{
    Py_ssize_t r = 0;           /* becomes 1 if any 1 bits are shifted off */

    assert(n >= 0);
    while (n >= 64) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

static void
reverse_sortslice(sortslice *s, Py_ssize_t n)
{
    reverse_slice(s->keys, &s->keys[n]);
    if (s->values != NULL)
        reverse_slice(s->values, &s->values[n]);
}

/* Here we define custom comparison functions to optimize for the cases one commonly
 * encounters in practice: homogeneous lists, often of one of the basic types. */

 /*
  * 在这里，我们定义了自定义比较函数，以针对实践中经常遇到的情况进行优化：
  * 同构列表，通常是基本类型之一
 */

 /* This struct holds the comparison function and helper functions
  * selected in the pre-sort check. */

  /*
   * 此结构保存在pre-sort check中选择的比较函数和帮助器
  */

  /* These are the special case compare functions.
   * ms->key_compare will always point to one of these: */

   /*
	* 这些是特例比较函数。
	* ms->key_compare将始终指向以下选项之一：
   */

   /* Heterogeneous compare: default, always safe to fall back on. */

   /*
	* 异类比较：默认，总是安全的。
   */
static int
safe_object_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    /* No assumptions necessary! */
    return PyObject_RichCompareBool(v, w, Py_LT);
}

/* Homogeneous compare: safe for any two compareable objects of the same type.
 * (ms->key_richcompare is set to ob_type->tp_richcompare in the
 *  pre-sort check.)
 */
 /*
  * 同质比较：对于同一类型的任何两个可比较对象都是安全的。
 */
static int
unsafe_object_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    PyObject *res_obj; int res;

    /* No assumptions, because we check first: */
    if (v->ob_type->tp_richcompare != ms->key_richcompare)
        return PyObject_RichCompareBool(v, w, Py_LT);

    assert(ms->key_richcompare != NULL);
    res_obj = (*(ms->key_richcompare))(v, w, Py_LT);

    if (res_obj == Py_NotImplemented) {
        Py_DECREF(res_obj);
        return PyObject_RichCompareBool(v, w, Py_LT);
    }
    if (res_obj == NULL)
        return -1;

    if (PyBool_Check(res_obj)) {
        res = (res_obj == Py_True);
    }
    else {
        res = PyObject_IsTrue(res_obj);
    }
    Py_DECREF(res_obj);

    /* Note that we can't assert
     *     res == PyObject_RichCompareBool(v, w, Py_LT);
     * because of evil compare functions like this:
     *     lambda a, b:  int(random.random() * 3) - 1)
     * (which is actually in test_sort.py) */
    return res;
}

/* Latin string compare: safe for any two latin (one byte per char) strings. */
static int
unsafe_latin_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    Py_ssize_t len;
    int res;

    /* Modified from Objects/unicodeobject.c:unicode_compare, assuming: */
    assert(v->ob_type == w->ob_type);
    assert(v->ob_type == &PyUnicode_Type);
    assert(PyUnicode_KIND(v) == PyUnicode_KIND(w));
    assert(PyUnicode_KIND(v) == PyUnicode_1BYTE_KIND);

    len = Py_MIN(PyUnicode_GET_LENGTH(v), PyUnicode_GET_LENGTH(w));
    res = memcmp(PyUnicode_DATA(v), PyUnicode_DATA(w), len);

    res = (res != 0 ?
           res < 0 :
           PyUnicode_GET_LENGTH(v) < PyUnicode_GET_LENGTH(w));

    assert(res == PyObject_RichCompareBool(v, w, Py_LT));;
    return res;
}

/* Bounded int compare: compare any two longs that fit in a single machine word. */
/* 有界整数比较：比较适合单个机器字的任意两个长度。*/
static int
unsafe_long_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    PyLongObject *vl, *wl; sdigit v0, w0; int res;

    /* Modified from Objects/longobject.c:long_compare, assuming: */
    assert(v->ob_type == w->ob_type);
    assert(v->ob_type == &PyLong_Type);
    assert(Py_ABS(Py_SIZE(v)) <= 1);
    assert(Py_ABS(Py_SIZE(w)) <= 1);

    vl = (PyLongObject*)v;
    wl = (PyLongObject*)w;

    v0 = Py_SIZE(vl) == 0 ? 0 : (sdigit)vl->ob_digit[0];
    w0 = Py_SIZE(wl) == 0 ? 0 : (sdigit)wl->ob_digit[0];

    if (Py_SIZE(vl) < 0)
        v0 = -v0;
    if (Py_SIZE(wl) < 0)
        w0 = -w0;

    res = v0 < w0;
    assert(res == PyObject_RichCompareBool(v, w, Py_LT));
    return res;
}

/* Float compare: compare any two floats. */
/* 浮点比较：比较任意两个浮点。 */
static int
unsafe_float_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    int res;

    /* Modified from Objects/floatobject.c:float_richcompare, assuming: */
    assert(v->ob_type == w->ob_type);
    assert(v->ob_type == &PyFloat_Type);

    res = PyFloat_AS_DOUBLE(v) < PyFloat_AS_DOUBLE(w);
    assert(res == PyObject_RichCompareBool(v, w, Py_LT));
    return res;
}

/* Tuple compare: compare *any* two tuples, using
 * ms->tuple_elem_compare to compare the first elements, which is set
 * using the same pre-sort check as we use for ms->key_compare,
 * but run on the list [x[0] for x in L]. This allows us to optimize compares
 * on two levels (as long as [x[0] for x in L] is type-homogeneous.) The idea is
 * that most tuple compares don't involve x[1:]. */
 /*
  * 元组比较：比较*任意*两个元组，使用 ms->tuple_elem_compare比较第一个元素,
  * 使用与ms->key_compare相同的预排序检查进行设置，但在列表[x[0] for x in L]上运行，
  * 这使我们能够在两个级别上优化比较（只要[x[0] for x in L]是类型同构的。）
  * 其思想是大多数元组比较不涉及x[1:]。
 */
static int
unsafe_tuple_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    PyTupleObject *vt, *wt;
    Py_ssize_t i, vlen, wlen;
    int k;

    /* Modified from Objects/tupleobject.c:tuplerichcompare, assuming: */
    assert(v->ob_type == w->ob_type);
    assert(v->ob_type == &PyTuple_Type);
    assert(Py_SIZE(v) > 0);
    assert(Py_SIZE(w) > 0);

    vt = (PyTupleObject *)v;
    wt = (PyTupleObject *)w;

    vlen = Py_SIZE(vt);
    wlen = Py_SIZE(wt);

    for (i = 0; i < vlen && i < wlen; i++) {
        k = PyObject_RichCompareBool(vt->ob_item[i], wt->ob_item[i], Py_EQ);
        if (k < 0)
            return -1;
        if (!k)
            break;
    }

    if (i >= vlen || i >= wlen)
        return vlen < wlen;

    if (i == 0)
        return ms->tuple_elem_compare(vt->ob_item[i], wt->ob_item[i], ms);
    else
        return PyObject_RichCompareBool(vt->ob_item[i], wt->ob_item[i], Py_LT);
}

/* An adaptive, stable, natural mergesort.  See listsort.txt.
 * Returns Py_None on success, NULL on error.  Even in case of error, the
 * list will be some permutation of its input state (nothing is lost or
 * duplicated).
 */
 /*
  * 自适应、稳定、自然的排序
  * 成功时返回Py_None，错误时返回NULL。
  * 即使在出现错误的情况下，列表也将是其输入状态的某种排列（没有任何内容丢失或重复）
 */

/*[clinic input]
list.sort

    *
    key as keyfunc: object = None
    reverse: bool(accept={int}) = False

Stable sort *IN PLACE*.
[clinic start generated code]*/

static PyObject *
list_sort_impl(PyListObject *self, PyObject *keyfunc, int reverse)
/*[clinic end generated code: output=57b9f9c5e23fbe42 input=b0fcf743982c5b90]*/
{
    MergeState ms;
    Py_ssize_t nremaining;
    Py_ssize_t minrun;
    sortslice lo;
    Py_ssize_t saved_ob_size, saved_allocated;
    PyObject **saved_ob_item;
    PyObject **final_ob_item;
    PyObject *result = NULL;            /* guilty until proved innocent */
    Py_ssize_t i;
    PyObject **keys;

    assert(self != NULL);
    assert(PyList_Check(self));
    if (keyfunc == Py_None)
        keyfunc = NULL;

    /* The list is temporarily made empty, so that mutations performed
     * by comparison functions can't affect the slice of memory we're
     * sorting (allowing mutations during sorting is a core-dump
     * factory, since ob_item may change).
     */
	 /*
	  * 列表暂时变为空，这样比较函数执行的突变不会影响我们正在排序的内存片
	  *（在排序期间允许突变是一个核心转储工厂，因为ob_项可能会更改）。
	 */
    saved_ob_size = Py_SIZE(self);
    saved_ob_item = self->ob_item;
    saved_allocated = self->allocated;
    Py_SIZE(self) = 0;
    self->ob_item = NULL;
    self->allocated = -1; /* any operation will reset it to >= 0 */

    if (keyfunc == NULL) {
        keys = NULL;
        lo.keys = saved_ob_item;
        lo.values = NULL;
    }
    else {
        if (saved_ob_size < MERGESTATE_TEMP_SIZE/2)
            /* Leverage stack space we allocated but won't otherwise use */
            keys = &ms.temparray[saved_ob_size+1];
        else {
            keys = PyMem_MALLOC(sizeof(PyObject *) * saved_ob_size);
            if (keys == NULL) {
                PyErr_NoMemory();
                goto keyfunc_fail;
            }
        }

        for (i = 0; i < saved_ob_size ; i++) {
            keys[i] = PyObject_CallFunctionObjArgs(keyfunc, saved_ob_item[i],
                                                   NULL);
            if (keys[i] == NULL) {
                for (i=i-1 ; i>=0 ; i--)
                    Py_DECREF(keys[i]);
                if (saved_ob_size >= MERGESTATE_TEMP_SIZE/2)
                    PyMem_FREE(keys);
                goto keyfunc_fail;
            }
        }

        lo.keys = keys;
        lo.values = saved_ob_item;
    }


    /* The pre-sort check: here's where we decide which compare function to use.
     * How much optimization is safe? We test for homogeneity with respect to
     * several properties that are expensive to check at compare-time, and
     * set ms appropriately. */
	 /*
	  * 预排序检查：这里是我们决定使用哪个比较函数的地方。多少优化是安全的？
	  * 我们测试了在比较时需要花费大量时间检查的几个属性的同质性，并适当地设置ms。
	 */
    if (saved_ob_size > 1) {
        /* Assume the first element is representative of the whole list. */
        int keys_are_in_tuples = (lo.keys[0]->ob_type == &PyTuple_Type &&
                                  Py_SIZE(lo.keys[0]) > 0);

        PyTypeObject* key_type = (keys_are_in_tuples ?
                                  PyTuple_GET_ITEM(lo.keys[0], 0)->ob_type :
                                  lo.keys[0]->ob_type);

        int keys_are_all_same_type = 1;
        int strings_are_latin = 1;
        int ints_are_bounded = 1;

        /* Prove that assumption by checking every key. */
        for (i=0; i < saved_ob_size; i++) {

            if (keys_are_in_tuples &&
                !(lo.keys[i]->ob_type == &PyTuple_Type && Py_SIZE(lo.keys[i]) != 0)) {
                keys_are_in_tuples = 0;
                keys_are_all_same_type = 0;
                break;
            }

            /* Note: for lists of tuples, key is the first element of the tuple
             * lo.keys[i], not lo.keys[i] itself! We verify type-homogeneity
             * for lists of tuples in the if-statement directly above. */
			 /*
			  * 注意：对于元组列表，key是元组lo.keys[i]的第一个元素，而不是lo.keys[i]本身！
			  * 我们在上面的if语句中验证元组列表的类型同质性。
			 */
			PyObject *key = (keys_are_in_tuples ?
                             PyTuple_GET_ITEM(lo.keys[i], 0) :
                             lo.keys[i]);

            if (key->ob_type != key_type) {
                keys_are_all_same_type = 0;
                /* If keys are in tuple we must loop over the whole list to make
                   sure all items are tuples */
				/*如果键在元组中，我们必须循环整个列表，以确保所有项都是元组*/
                if (!keys_are_in_tuples) {
                    break;
                }
            }

            if (keys_are_all_same_type) {
                if (key_type == &PyLong_Type &&
                    ints_are_bounded &&
                    Py_ABS(Py_SIZE(key)) > 1) {

                    ints_are_bounded = 0;
                }
                else if (key_type == &PyUnicode_Type &&
                         strings_are_latin &&
                         PyUnicode_KIND(key) != PyUnicode_1BYTE_KIND) {

                        strings_are_latin = 0;
                    }
                }
            }

        /* Choose the best compare, given what we now know about the keys. */

        if (keys_are_all_same_type) {

            if (key_type == &PyUnicode_Type && strings_are_latin) {
                ms.key_compare = unsafe_latin_compare;
            }
            else if (key_type == &PyLong_Type && ints_are_bounded) {
                ms.key_compare = unsafe_long_compare;
            }
            else if (key_type == &PyFloat_Type) {
                ms.key_compare = unsafe_float_compare;
            }
            else if ((ms.key_richcompare = key_type->tp_richcompare) != NULL) {
                ms.key_compare = unsafe_object_compare;
            }
            else {
                ms.key_compare = safe_object_compare;
            }
        }
        else {
            ms.key_compare = safe_object_compare;
        }

        if (keys_are_in_tuples) {
            /* Make sure we're not dealing with tuples of tuples
             * (remember: here, key_type refers list [key[0] for key in keys]) */
			 /*确保我们没有处理元组的元组（请记住：这里，key_type指的是列表[key[0]表示key-in-key]）*/
			if (key_type == &PyTuple_Type) {
                ms.tuple_elem_compare = safe_object_compare;
            }
            else {
                ms.tuple_elem_compare = ms.key_compare;
            }

            ms.key_compare = unsafe_tuple_compare;
        }
    }
    /* End of pre-sort check: ms is now set properly! */

    merge_init(&ms, saved_ob_size, keys != NULL);

    nremaining = saved_ob_size;
    if (nremaining < 2)
        goto succeed;

    /* Reverse sort stability achieved by initially reversing the list,
    applying a stable forward sort, then reversing the final result. */
	/*
	 * 通过最初反转列表实现反向排序稳定性，应用稳定的正向排序，然后反转最终结果。
	*/
    if (reverse) {
        if (keys != NULL)
            reverse_slice(&keys[0], &keys[saved_ob_size]);
        reverse_slice(&saved_ob_item[0], &saved_ob_item[saved_ob_size]);
    }

    /* March over the array once, left to right, finding natural runs,
     * and extending short natural runs to minrun elements.
     */
	 /*
	  * 从左到右在阵列上行进一次，查找自然行程，并将短自然行程延伸到minrun元素。
	  */
    minrun = merge_compute_minrun(nremaining);
    do {
        int descending;
        Py_ssize_t n;

        /* Identify next run. */
        n = count_run(&ms, lo.keys, lo.keys + nremaining, &descending);
        if (n < 0)
            goto fail;
        if (descending)
            reverse_sortslice(&lo, n);
        /* If short, extend to min(minrun, nremaining). */
        if (n < minrun) {
            const Py_ssize_t force = nremaining <= minrun ?
                              nremaining : minrun;
            if (binarysort(&ms, lo, lo.keys + force, lo.keys + n) < 0)
                goto fail;
            n = force;
        }
        /* Push run onto pending-runs stack, and maybe merge. */
        assert(ms.n < MAX_MERGE_PENDING);
        ms.pending[ms.n].base = lo;
        ms.pending[ms.n].len = n;
        ++ms.n;
        if (merge_collapse(&ms) < 0)
            goto fail;
        /* Advance to find next run. */
        sortslice_advance(&lo, n);
        nremaining -= n;
    } while (nremaining);

    if (merge_force_collapse(&ms) < 0)
        goto fail;
    assert(ms.n == 1);
    assert(keys == NULL
           ? ms.pending[0].base.keys == saved_ob_item
           : ms.pending[0].base.keys == &keys[0]);
    assert(ms.pending[0].len == saved_ob_size);
    lo = ms.pending[0].base;

succeed:
    result = Py_None;
fail:
    if (keys != NULL) {
        for (i = 0; i < saved_ob_size; i++)
            Py_DECREF(keys[i]);
        if (saved_ob_size >= MERGESTATE_TEMP_SIZE/2)
            PyMem_FREE(keys);
    }

    if (self->allocated != -1 && result != NULL) {
        /* The user mucked with the list during the sort,
         * and we don't already have another error to report.
         */
        PyErr_SetString(PyExc_ValueError, "list modified during sort");
        result = NULL;
    }

    if (reverse && saved_ob_size > 1)
        reverse_slice(saved_ob_item, saved_ob_item + saved_ob_size);

    merge_freemem(&ms);

keyfunc_fail:
    final_ob_item = self->ob_item;
    i = Py_SIZE(self);
    Py_SIZE(self) = saved_ob_size;
    self->ob_item = saved_ob_item;
    self->allocated = saved_allocated;
    if (final_ob_item != NULL) {
        /* we cannot use _list_clear() for this because it does not
           guarantee that the list is really empty when it returns */
		   /*我们不能对此使用_list_clear（），因为它不能保证列表返回时确实为空*/
		while (--i >= 0) {
            Py_XDECREF(final_ob_item[i]);
        }
        PyMem_FREE(final_ob_item);
    }
    Py_XINCREF(result);
    return result;
}
#undef IFLT
#undef ISLT

int
PyList_Sort(PyObject *v)
{
    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    v = list_sort_impl((PyListObject *)v, NULL, 0);
    if (v == NULL)
        return -1;
    Py_DECREF(v);
    return 0;
}

/*[clinic input]
list.reverse

Reverse *IN PLACE*.
[clinic start generated code]*/

static PyObject *
list_reverse_impl(PyListObject *self)
/*[clinic end generated code: output=482544fc451abea9 input=eefd4c3ae1bc9887]*/
{
    if (Py_SIZE(self) > 1)
        reverse_slice(self->ob_item, self->ob_item + Py_SIZE(self));
    Py_RETURN_NONE;
}

int
PyList_Reverse(PyObject *v)
{
    PyListObject *self = (PyListObject *)v;

    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (Py_SIZE(self) > 1)
        reverse_slice(self->ob_item, self->ob_item + Py_SIZE(self));
    return 0;
}

PyObject *
PyList_AsTuple(PyObject *v)
{
    PyObject *w;
    PyObject **p, **q;
    Py_ssize_t n;
    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    n = Py_SIZE(v);
    w = PyTuple_New(n);
    if (w == NULL)
        return NULL;
    p = ((PyTupleObject *)w)->ob_item;
    q = ((PyListObject *)v)->ob_item;
    while (--n >= 0) {
        Py_INCREF(*q);
        *p = *q;
        p++;
        q++;
    }
    return w;
}

/*[clinic input]
list.index

    value: object
    start: slice_index(accept={int}) = 0
    stop: slice_index(accept={int}, c_default="PY_SSIZE_T_MAX") = sys.maxsize
    /

Return first index of value.

Raises ValueError if the value is not present.
[clinic start generated code]*/

static PyObject *
list_index_impl(PyListObject *self, PyObject *value, Py_ssize_t start,
                Py_ssize_t stop)
/*[clinic end generated code: output=ec51b88787e4e481 input=40ec5826303a0eb1]*/
{
    Py_ssize_t i;

    if (start < 0) {
        start += Py_SIZE(self);
        if (start < 0)
            start = 0;
    }
    if (stop < 0) {
        stop += Py_SIZE(self);
        if (stop < 0)
            stop = 0;
    }
    for (i = start; i < stop && i < Py_SIZE(self); i++) {
        PyObject *obj = self->ob_item[i];
        Py_INCREF(obj);
        int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
        Py_DECREF(obj);
        if (cmp > 0)
            return PyLong_FromSsize_t(i);
        else if (cmp < 0)
            return NULL;
    }
    PyErr_Format(PyExc_ValueError, "%R is not in list", value);
    return NULL;
}

/*[clinic input]
list.count

     value: object
     /

Return number of occurrences of value.
[clinic start generated code]*/

static PyObject *
list_count(PyListObject *self, PyObject *value)
/*[clinic end generated code: output=b1f5d284205ae714 input=3bdc3a5e6f749565]*/
{
    Py_ssize_t count = 0;
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *obj = self->ob_item[i];
        if (obj == value) {
           count++;
           continue;
        }
        Py_INCREF(obj);
        int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
        Py_DECREF(obj);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return NULL;
    }
    return PyLong_FromSsize_t(count);
}

/*[clinic input]
list.remove

     value: object
     /

Remove first occurrence of value.

Raises ValueError if the value is not present.
[clinic start generated code]*/

static PyObject *
list_remove(PyListObject *self, PyObject *value)
/*[clinic end generated code: output=f087e1951a5e30d1 input=2dc2ba5bb2fb1f82]*/
{
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *obj = self->ob_item[i];
        Py_INCREF(obj);
        int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
        Py_DECREF(obj);
        if (cmp > 0) {
            if (list_ass_slice(self, i, i+1,
                               (PyObject *)NULL) == 0)
                Py_RETURN_NONE;
            return NULL;
        }
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "list.remove(x): x not in list");
    return NULL;
}

static int
list_traverse(PyListObject *o, visitproc visit, void *arg)
{
    Py_ssize_t i;

    for (i = Py_SIZE(o); --i >= 0; )
        Py_VISIT(o->ob_item[i]);
    return 0;
}

static PyObject *
list_richcompare(PyObject *v, PyObject *w, int op)
{
    PyListObject *vl, *wl;
    Py_ssize_t i;

    if (!PyList_Check(v) || !PyList_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    vl = (PyListObject *)v;
    wl = (PyListObject *)w;

    if (Py_SIZE(vl) != Py_SIZE(wl) && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the lists differ */
        if (op == Py_EQ)
            Py_RETURN_FALSE;
        else
            Py_RETURN_TRUE;
    }

    /* Search for the first index where items are different */
    for (i = 0; i < Py_SIZE(vl) && i < Py_SIZE(wl); i++) {
        PyObject *vitem = vl->ob_item[i];
        PyObject *witem = wl->ob_item[i];
        if (vitem == witem) {
            continue;
        }

        Py_INCREF(vitem);
        Py_INCREF(witem);
        int k = PyObject_RichCompareBool(vl->ob_item[i],
                                         wl->ob_item[i], Py_EQ);
        Py_DECREF(vitem);
        Py_DECREF(witem);
        if (k < 0)
            return NULL;
        if (!k)
            break;
    }

    if (i >= Py_SIZE(vl) || i >= Py_SIZE(wl)) {
        /* No more items to compare -- compare sizes */
        Py_RETURN_RICHCOMPARE(Py_SIZE(vl), Py_SIZE(wl), op);
    }

    /* We have an item that differs -- shortcuts for EQ/NE */
    if (op == Py_EQ) {
        Py_RETURN_FALSE;
    }
    if (op == Py_NE) {
        Py_RETURN_TRUE;
    }

    /* Compare the final item again using the proper operator */
    return PyObject_RichCompare(vl->ob_item[i], wl->ob_item[i], op);
}

/*[clinic input]
list.__init__

    iterable: object(c_default="NULL") = ()
    /

Built-in mutable sequence.

If no argument is given, the constructor creates a new empty list.
The argument must be an iterable if specified.

如果没有给出参数，构造函数将创建一个新的空列表。
如果指定，则参数必须是iterable。

[clinic start generated code]*/

static int
list___init___impl(PyListObject *self, PyObject *iterable)
/*[clinic end generated code: output=0f3c21379d01de48 input=b3f3fe7206af8f6b]*/
{
    /* Verify list invariants established by PyType_GenericAlloc() */
    assert(0 <= Py_SIZE(self));
    assert(Py_SIZE(self) <= self->allocated || self->allocated == -1);
    assert(self->ob_item != NULL ||
           self->allocated == 0 || self->allocated == -1);

    /* Empty previous contents */
    if (self->ob_item != NULL) {
        (void)_list_clear(self);
    }
    if (iterable != NULL) {
        PyObject *rv = list_extend(self, iterable);
        if (rv == NULL)
            return -1;
        Py_DECREF(rv);
    }

#ifdef Py_REF_DEBUG
	trace_list_init(self);
#endif
	
    return 0;
}

/*[clinic input]
list.__sizeof__

Return the size of the list in memory, in bytes.
[clinic start generated code]*/

static PyObject *
list___sizeof___impl(PyListObject *self)
/*[clinic end generated code: output=3417541f95f9a53e input=b8030a5d5ce8a187]*/
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(self)) + self->allocated * sizeof(void*);
    return PyLong_FromSsize_t(res);
}

static PyObject *list_iter(PyObject *seq);
static PyObject *list_subscript(PyListObject*, PyObject*);

static PyMethodDef list_methods[] = {
    {"__getitem__", (PyCFunction)list_subscript, METH_O|METH_COEXIST, "x.__getitem__(y) <==> x[y]"},
    LIST___REVERSED___METHODDEF
    LIST___SIZEOF___METHODDEF
    LIST_CLEAR_METHODDEF
    LIST_COPY_METHODDEF
    LIST_APPEND_METHODDEF
    LIST_INSERT_METHODDEF
    LIST_EXTEND_METHODDEF
    LIST_POP_METHODDEF
    LIST_REMOVE_METHODDEF
    LIST_INDEX_METHODDEF
    LIST_COUNT_METHODDEF
    LIST_REVERSE_METHODDEF
    LIST_SORT_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PySequenceMethods list_as_sequence = {
    (lenfunc)list_length,                       /* sq_length */
    (binaryfunc)list_concat,                    /* sq_concat */
    (ssizeargfunc)list_repeat,                  /* sq_repeat */
    (ssizeargfunc)list_item,                    /* sq_item */
    0,                                          /* sq_slice */
    (ssizeobjargproc)list_ass_item,             /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    (objobjproc)list_contains,                  /* sq_contains */
    (binaryfunc)list_inplace_concat,            /* sq_inplace_concat */
    (ssizeargfunc)list_inplace_repeat,          /* sq_inplace_repeat */
};

static PyObject *
list_subscript(PyListObject* self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyList_GET_SIZE(self);
        return list_item(self, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        PyObject* result;
        PyObject* it;
        PyObject **src, **dest;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(Py_SIZE(self), &start, &stop,
                                            step);

        if (slicelength <= 0) {
            return PyList_New(0);
        }
        else if (step == 1) {
            return list_slice(self, start, stop);
        }
        else {
            result = PyList_New(slicelength);
            if (!result) return NULL;

            src = self->ob_item;
            dest = ((PyListObject *)result)->ob_item;
            for (cur = start, i = 0; i < slicelength;
                 cur += (size_t)step, i++) {
                it = src[cur];
                Py_INCREF(it);
                dest[i] = it;
            }

            return result;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "list indices must be integers or slices, not %.200s",
                     item->ob_type->tp_name);
        return NULL;
    }
}

static int
list_ass_subscript(PyListObject* self, PyObject* item, PyObject* value)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += PyList_GET_SIZE(self);
        return list_ass_item(self, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelength = PySlice_AdjustIndices(Py_SIZE(self), &start, &stop,
                                            step);

        if (step == 1)
            return list_ass_slice(self, start, stop, value);

        /* Make sure s[5:2] = [..] inserts at the right place:
           before 5, not before 2. */
        if ((step < 0 && start < stop) ||
            (step > 0 && start > stop))
            stop = start;

        if (value == NULL) {
            /* delete slice */
            PyObject **garbage;
            size_t cur;
            Py_ssize_t i;
            int res;

            if (slicelength <= 0)
                return 0;

            if (step < 0) {
                stop = start + 1;
                start = stop + step*(slicelength - 1) - 1;
                step = -step;
            }

            garbage = (PyObject**)
                PyMem_MALLOC(slicelength*sizeof(PyObject*));
            if (!garbage) {
                PyErr_NoMemory();
                return -1;
            }

            /* drawing pictures might help understand these for
               loops. Basically, we memmove the parts of the
               list that are *not* part of the slice: step-1
               items for each item that is part of the slice,
               and then tail end of the list that was not
               covered by the slice */
			   /*
				* 基本上，我们移动列表中*不是*部分的部分：
				* 为每个属于该部分的step-1个项目移动项目，然后移动该部分未覆盖的列表末尾
			   */
            for (cur = start, i = 0;
                 cur < (size_t)stop;
                 cur += step, i++) {
                Py_ssize_t lim = step - 1;

                garbage[i] = PyList_GET_ITEM(self, cur);

                if (cur + step >= (size_t)Py_SIZE(self)) {
                    lim = Py_SIZE(self) - cur - 1;
                }

                memmove(self->ob_item + cur - i,
                    self->ob_item + cur + 1,
                    lim * sizeof(PyObject *));
            }
            cur = start + (size_t)slicelength * step;
            if (cur < (size_t)Py_SIZE(self)) {
                memmove(self->ob_item + cur - slicelength,
                    self->ob_item + cur,
                    (Py_SIZE(self) - cur) *
                     sizeof(PyObject *));
            }

            Py_SIZE(self) -= slicelength;
            res = list_resize(self, Py_SIZE(self));

            for (i = 0; i < slicelength; i++) {
                Py_DECREF(garbage[i]);
            }
            PyMem_FREE(garbage);

            return res;
        }
        else {
            /* assign slice */
            PyObject *ins, *seq;
            PyObject **garbage, **seqitems, **selfitems;
            Py_ssize_t i;
            size_t cur;

            /* protect against a[::-1] = a */
            if (self == (PyListObject*)value) {
                seq = list_slice((PyListObject*)value, 0,
                                   PyList_GET_SIZE(value));
            }
            else {
                seq = PySequence_Fast(value,
                                      "must assign iterable "
                                      "to extended slice");
            }
            if (!seq)
                return -1;

            if (PySequence_Fast_GET_SIZE(seq) != slicelength) {
                PyErr_Format(PyExc_ValueError,
                    "attempt to assign sequence of "
                    "size %zd to extended slice of "
                    "size %zd",
                         PySequence_Fast_GET_SIZE(seq),
                         slicelength);
                Py_DECREF(seq);
                return -1;
            }

            if (!slicelength) {
                Py_DECREF(seq);
                return 0;
            }

            garbage = (PyObject**)
                PyMem_MALLOC(slicelength*sizeof(PyObject*));
            if (!garbage) {
                Py_DECREF(seq);
                PyErr_NoMemory();
                return -1;
            }

            selfitems = self->ob_item;
            seqitems = PySequence_Fast_ITEMS(seq);
            for (cur = start, i = 0; i < slicelength;
                 cur += (size_t)step, i++) {
                garbage[i] = selfitems[cur];
                ins = seqitems[i];
                Py_INCREF(ins);
                selfitems[cur] = ins;
            }

            for (i = 0; i < slicelength; i++) {
                Py_DECREF(garbage[i]);
            }

            PyMem_FREE(garbage);
            Py_DECREF(seq);

            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "list indices must be integers or slices, not %.200s",
                     item->ob_type->tp_name);
        return -1;
    }
}

static PyMappingMethods list_as_mapping = {
    (lenfunc)list_length,
    (binaryfunc)list_subscript,
    (objobjargproc)list_ass_subscript
};

PyTypeObject PyList_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list",
    sizeof(PyListObject),
    0,
    (destructor)list_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)list_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    &list_as_sequence,                          /* tp_as_sequence */
    &list_as_mapping,                           /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_LIST_SUBCLASS, /* tp_flags */
    list___init____doc__,                       /* tp_doc */
    (traverseproc)list_traverse,                /* tp_traverse */
    (inquiry)_list_clear,                       /* tp_clear */
    list_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    list_iter,                                  /* tp_iter */
    0,                                          /* tp_iternext */
    list_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)list___init__,                    /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

/*********************** List Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} listiterobject;

static void listiter_dealloc(listiterobject *);
static int listiter_traverse(listiterobject *, visitproc, void *);
static PyObject *listiter_next(listiterobject *);
static PyObject *listiter_len(listiterobject *);
static PyObject *listiter_reduce_general(void *_it, int forward);
static PyObject *listiter_reduce(listiterobject *);
static PyObject *listiter_setstate(listiterobject *, PyObject *state);

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");
PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef listiter_methods[] = {
    {"__length_hint__", (PyCFunction)listiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)listiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)listiter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyListIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list_iterator",                            /* tp_name */
    sizeof(listiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)listiter_dealloc,               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)listiter_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)listiter_next,                /* tp_iternext */
    listiter_methods,                           /* tp_methods */
    0,                                          /* tp_members */
};


static PyObject *
list_iter(PyObject *seq)
{
    listiterobject *it;

    if (!PyList_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_GC_New(listiterobject, &PyListIter_Type);
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    Py_INCREF(seq);
    it->it_seq = (PyListObject *)seq;
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

static void
listiter_dealloc(listiterobject *it)
{
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
listiter_traverse(listiterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
listiter_next(listiterobject *it)
{
    PyListObject *seq;
    PyObject *item;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    assert(PyList_Check(seq));

    if (it->it_index < PyList_GET_SIZE(seq)) {
        item = PyList_GET_ITEM(seq, it->it_index);
        ++it->it_index;
        Py_INCREF(item);
        return item;
    }

    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
listiter_len(listiterobject *it)
{
    Py_ssize_t len;
    if (it->it_seq) {
        len = PyList_GET_SIZE(it->it_seq) - it->it_index;
        if (len >= 0)
            return PyLong_FromSsize_t(len);
    }
    return PyLong_FromLong(0);
}

static PyObject *
listiter_reduce(listiterobject *it)
{
    return listiter_reduce_general(it, 1);
}

static PyObject *
listiter_setstate(listiterobject *it, PyObject *state)
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyList_GET_SIZE(it->it_seq))
            index = PyList_GET_SIZE(it->it_seq); /* iterator exhausted */
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

/*********************** List Reverse Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} listreviterobject;

static void listreviter_dealloc(listreviterobject *);
static int listreviter_traverse(listreviterobject *, visitproc, void *);
static PyObject *listreviter_next(listreviterobject *);
static PyObject *listreviter_len(listreviterobject *);
static PyObject *listreviter_reduce(listreviterobject *);
static PyObject *listreviter_setstate(listreviterobject *, PyObject *);

static PyMethodDef listreviter_methods[] = {
    {"__length_hint__", (PyCFunction)listreviter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)listreviter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)listreviter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyListRevIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list_reverseiterator",                     /* tp_name */
    sizeof(listreviterobject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)listreviter_dealloc,            /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)listreviter_traverse,         /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)listreviter_next,             /* tp_iternext */
    listreviter_methods,                /* tp_methods */
    0,
};

/*[clinic input]
list.__reversed__

Return a reverse iterator over the list.
[clinic start generated code]*/

static PyObject *
list___reversed___impl(PyListObject *self)
/*[clinic end generated code: output=b166f073208c888c input=eadb6e17f8a6a280]*/
{
    listreviterobject *it;

    it = PyObject_GC_New(listreviterobject, &PyListRevIter_Type);
    if (it == NULL)
        return NULL;
    assert(PyList_Check(self));
    it->it_index = PyList_GET_SIZE(self) - 1;
    Py_INCREF(self);
    it->it_seq = self;
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static void
listreviter_dealloc(listreviterobject *it)
{
    PyObject_GC_UnTrack(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
listreviter_traverse(listreviterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
listreviter_next(listreviterobject *it)
{
    PyObject *item;
    Py_ssize_t index;
    PyListObject *seq;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL) {
        return NULL;
    }
    assert(PyList_Check(seq));

    index = it->it_index;
    if (index>=0 && index < PyList_GET_SIZE(seq)) {
        item = PyList_GET_ITEM(seq, index);
        it->it_index--;
        Py_INCREF(item);
        return item;
    }
    it->it_index = -1;
    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
listreviter_len(listreviterobject *it)
{
    Py_ssize_t len = it->it_index + 1;
    if (it->it_seq == NULL || PyList_GET_SIZE(it->it_seq) < len)
        len = 0;
    return PyLong_FromSsize_t(len);
}

static PyObject *
listreviter_reduce(listreviterobject *it)
{
    return listiter_reduce_general(it, 0);
}

static PyObject *
listreviter_setstate(listreviterobject *it, PyObject *state)
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < -1)
            index = -1;
        else if (index > PyList_GET_SIZE(it->it_seq) - 1)
            index = PyList_GET_SIZE(it->it_seq) - 1;
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

/* common pickling support */

static PyObject *
listiter_reduce_general(void *_it, int forward)
{
    PyObject *list;

    /* the objects are not the same, index is of different types! */
    if (forward) {
        listiterobject *it = (listiterobject *)_it;
        if (it->it_seq)
            return Py_BuildValue("N(O)n", _PyObject_GetBuiltin("iter"),
                                 it->it_seq, it->it_index);
    } else {
        listreviterobject *it = (listreviterobject *)_it;
        if (it->it_seq)
            return Py_BuildValue("N(O)n", _PyObject_GetBuiltin("reversed"),
                                 it->it_seq, it->it_index);
    }
    /* empty iterator, create an empty list */
    list = PyList_New(0);
    if (list == NULL)
        return NULL;
    return Py_BuildValue("N(N)", _PyObject_GetBuiltin("iter"), list);
}

#ifdef Py_REF_DEBUG
/* trace list's attributes when a new list is created */
static
void trace_list_new(Py_ssize_t size, PyListObject *op)
{
	Py_MyDebug_List_Create(size, op);
}


static void
trace_list_init(PyListObject *op)
{
	Py_MyDebug_List_Init(op);
}

static void
set_item_hit(PyObject* obj)
{
	Py_MyDebug_List_Setitem(obj);
}

/* you can use this function to observe Python list's resize strategy */
static void
append_item_hit(PyListObject* obj, PyObject* new_item)
{
	Py_MyDebug_List_Appenditem(obj, new_item);
}


void
_find_free_list(PyListObject *a)
{
	for (int i = 0; i < PyList_MAXFREELIST; i++) {
		if (free_list[i] != NULL) {
			printf("free_list[%d]'s address: @%p ", i, free_list[i]);
			if (free_list[i] == a) {
				printf("[hold]");
			}
			printf("\n");
		}
		
	}

}

static void
trace_free_list(PyListObject *a) 
{

	/* len(list) will trigger this function and size(list) == 2*/

	if (_Py_GetMy_Debug() == Py_MYDEBUG_LIST || _Py_GetMy_Debug() == Py_MYDEBUG_ALL)
	{
		/* reduce useless info to print */
		if (a->ob_base.ob_size != 2) {
			return;
		}

		if (numfree > 0) {
			printf("now free_list has %d ListObject can used\n", numfree);
			PyObject_Print((PyObject *)a, stdout, 1);
			printf(" address is @%p\n", a);
			_find_free_list(a);
		}
	}
}


#endif
