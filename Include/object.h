#ifndef Py_OBJECT_H
#define Py_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* 
Object and type object interface 
Object 和 TypeObject 的接口
*/

/*
Objects are structures allocated on the heap.  Special rules apply to
the use of objects to ensure they are properly garbage-collected.
Objects are never allocated statically or on the stack; they must be
accessed through special macros and functions only.  (Type objects are
exceptions to the first rule; the standard types are represented by
statically initialized type objects, although work on type/class unification
for Python 2.2 made it possible to have heap-allocated type objects too).

Object是在堆上分配的结构。特殊规则适用于Object的使用，
以确保它们被正确地垃圾收集。
Object从不静态分配或在栈上分配；只能通过特殊宏和函数访问它们。
（TypeObject是第一条规则的例外；标准类型由静态初始化的TypeObject表示，
尽管Python2.2的type/class统一工作也使堆分配类型对象成为可能）。
type/class: https://stackoverflow.com/questions/22445664/why-types-class-unification-majorly-in-python
PEP252

An object has a 'reference count' that is increased or decreased when a
pointer to the object is copied or deleted; when the reference count
reaches zero there are no references to the object left and it can be
removed from the heap.

复制或删除指向对象的指针时，对象的“引用计数”会增加或减少；
当引用计数达到零时，就没有对对象的引用了，可以将其从堆中删除。

An object has a 'type' that determines what it represents and what kind
of data it contains.  An object's type is fixed when it is created.
Types themselves are represented as objects; an object contains a
pointer to the corresponding type object.  The type itself has a type
pointer pointing to the object representing the type 'type', which
contains a pointer to itself!.

一个对象有一个“type”，它决定了它代表什么以及它包含什么样的数据。
对象的类型在创建时是固定的。type本身被表示为对象(TypeObject)；对象包含指向相应类型对象的指针(ob_type)。
类型本身有一个类型指针(ob_type)，指向表示类型“type”的对象，该类型包含一个指向自身的指针！

Objects do not float around in memory; once allocated an object keeps
the same size and address.  Objects that must hold variable-size data
can contain pointers to variable-size parts of the object.  Not all
objects of the same type have the same size; but the size cannot change
after allocation.  (These restrictions are made so a reference to an
object can be simply a pointer -- moving an object would require
updating all the pointers, and changing an object's size would require
moving it if there was another object right next to it.)

对象不会在内存中四处浮动；一旦分配，对象将保持相同的大小和地址。
必须保存 可变对象 可以包含指向对象可变大小部分的指针。
并非所有类型相同的对象都具有相同的大小；但分配后大小不能改变。
（这些限制使得对对象的引用可以仅仅是一个指针——移动一个对象需要更新所有指针，
如果对象旁边有另一个对象，则更改对象的大小需要移动它。）

Objects are always accessed through pointers of the type 'PyObject *'.
The type 'PyObject' is a structure that only contains the reference count
and the type pointer.  The actual memory allocated for an object
contains other data that can only be accessed after casting the pointer
to a pointer to a longer structure type.  This longer type must start
with the reference count and type fields; the macro PyObject_HEAD should be
used for this (to accommodate for future changes).  The implementation
of a particular object type can cast the object pointer to the proper
type and back.

对象总是通过“PyObject*”类型的指针访问。类型“PyObject”是一个仅包含引用计数（ob_refcnt）和类型指针（ob_type）的结构。
为对象分配的实际内存包含其他数据，这些数据只有在将指针转换为较长结构类型的指针后才能访问。
这个较长的类型必须以引用计数和类型字段开头（PyObject_HEAD）；应使用宏PyObject_头进行此操作（以适应将来的更改）。
特定对象类型的实现可以将对象指针强制转换为正确的类型并返回。

A standard interface exists for objects that contain an array of items
whose size is determined when the object is allocated.

对于包含项目数组（其大小在分配对象时确定）的对象，存在一个标准接口。

*/

/* Py_DEBUG implies Py_TRACE_REFS. */
#if defined(Py_DEBUG) && !defined(Py_TRACE_REFS)
#define Py_TRACE_REFS
#endif

/* Py_TRACE_REFS implies Py_REF_DEBUG. */
#if defined(Py_TRACE_REFS) && !defined(Py_REF_DEBUG)
#define Py_REF_DEBUG
#endif

#if defined(Py_LIMITED_API) && defined(Py_REF_DEBUG)
#error Py_LIMITED_API is incompatible with Py_DEBUG, Py_TRACE_REFS, and Py_REF_DEBUG
#endif


#ifdef Py_TRACE_REFS
/* 
Define pointers to support a doubly-linked list of all live heap objects. 
定义指针以支持所有活动堆对象的双链接列表。
*/
#define _PyObject_HEAD_EXTRA            \
    struct _object *_ob_next;           \
    struct _object *_ob_prev;

#define _PyObject_EXTRA_INIT 0, 0,

#else
#define _PyObject_HEAD_EXTRA
#define _PyObject_EXTRA_INIT
#endif

/* 
PyObject_HEAD defines the initial segment of every PyObject. 
PyObject_HEAD定义每个PyObject的初始段
*/
#define PyObject_HEAD                   PyObject ob_base;

#define PyObject_HEAD_INIT(type)        \
    { _PyObject_EXTRA_INIT              \
    1, type },

#define PyVarObject_HEAD_INIT(type, size)       \
    { PyObject_HEAD_INIT(type) size },

/* PyObject_VAR_HEAD defines the initial segment of all variable-size
 * container objects.  These end with a declaration of an array with 1
 * element, but enough space is malloc'ed so that the array actually
 * has room for ob_size elements.  Note that ob_size is an element count,
 * not necessarily a byte count.

 PyObject_VAR_HEAD定义所有 可变对象 的初始段。
 最后声明了一个包含1个元素的数组，但是有足够的空间被malloc，因此数组实际上有空间容纳ob_size的元素。
 请注意，ob_size是元素计数，不一定是字节计数。

 */
#define PyObject_VAR_HEAD      PyVarObject ob_base;
#define Py_INVALID_SIZE (Py_ssize_t)-1

/* Nothing is actually declared to be a PyObject, but every pointer to
 * a Python object can be cast to a PyObject*.  This is inheritance built
 * by hand.  Similarly every pointer to a variable-size Python object can,
 * in addition, be cast to PyVarObject*.

 实际上没有任何东西被声明为PyObject，但指向Python对象的每个指针都可以转换为PyObject*。
 这是手动实现继承机制（因为C不是面向对象的语言）。
 同样，每个指向 可变对象 的指针也可以强制转换为PyVarObject*。

 */
typedef struct _object {
    _PyObject_HEAD_EXTRA
    Py_ssize_t ob_refcnt;         /* 引用计数 */
    struct _typeobject *ob_type;  /* 类型指针 */
} PyObject;

typedef struct {
    PyObject ob_base;            /* 包含ob_refcnt和ob_type */
    Py_ssize_t ob_size; /* Number of items in variable part 可变对象可容纳的元素个数，大小可变 */
} PyVarObject;

#define Py_REFCNT(ob)           (((PyObject*)(ob))->ob_refcnt)
#define Py_TYPE(ob)             (((PyObject*)(ob))->ob_type)
#define Py_SIZE(ob)             (((PyVarObject*)(ob))->ob_size)

#ifndef Py_LIMITED_API
/********************* String Literals ****************************************/
/* This structure helps managing static strings. The basic usage goes like this:
   Instead of doing

       r = PyObject_CallMethod(o, "foo", "args", ...);

   do

       _Py_IDENTIFIER(foo);
       ...
       r = _PyObject_CallMethodId(o, &PyId_foo, "args", ...);

   此结构有助于管理静态字符串。基本用法应该是：
	_Py_IDENTIFIER(foo);
    ...
    r = _PyObject_CallMethodId(o, &PyId_foo, "args", ...);
   不能这么用：
    r = PyObject_CallMethod(o, "foo", "args", ...);

   PyId_foo is a static variable, either on block level or file level. On first
   usage, the string "foo" is interned, and the structures are linked. On interpreter
   shutdown, all strings are released (through _PyUnicode_ClearStaticStrings).

   PyId_foo是一个静态变量，在块级或文件级。第一次使用时，字符串“foo”被插入，结构被链接。
   解释器关闭时，释放所有字符串（通过_PyUnicode_ClearStaticStrings）。

   Alternatively, _Py_static_string allows choosing the variable name.
   _PyUnicode_FromId returns a borrowed reference to the interned string.
   _PyObject_{Get,Set,Has}AttrId are __getattr__ versions using _Py_Identifier*.

   _PyObject_{Get,Set,Has}AttrId 是使用 _Py_Identifier* 的 __getattr__ 版本 ??

   _Py_Identifier是表示标识符的结构体，标识符即变量名 
   python字符串有一个方法为：isidentifier()，用来鉴别一个字符串是否能成为标识符
   >>> '0xyz'.isidentifier() -> False
   >>> 'xyzABC'.isidentifier() -> True
*/
typedef struct _Py_Identifier {
    struct _Py_Identifier *next;
    const char* string;
    PyObject *object;
} _Py_Identifier;

#define _Py_static_string_init(value) { .next = NULL, .string = value, .object = NULL }
#define _Py_static_string(varname, value)  static _Py_Identifier varname = _Py_static_string_init(value)
#define _Py_IDENTIFIER(varname) _Py_static_string(PyId_##varname, #varname)

#endif /* !Py_LIMITED_API */

/*
Type objects contain a string containing the type name (to help somewhat
in debugging), the allocation parameters (see PyObject_New() and
PyObject_NewVar()),
and methods for accessing objects of the type.  Methods are optional, a
nil pointer meaning that particular kind of access is not available for
this type.  The Py_DECREF() macro uses the tp_dealloc method without
checking for a nil pointer; it should always be implemented except if
the implementation can guarantee that the reference count will never
reach zero (e.g., for statically allocated type objects).

类型对象包含一个字符串，其中包含类型名称（对调试有所帮助）、
分配参数（请参见PyObject_New（）和PyObject_NewVar（））
以及访问该类型对象的方法。方法是可选的，一个nil指针意味着特定类型的访问不可用于此类型。
Py_DECREF（）宏使用tp_dealloc方法，而不检查nil指针；
应该始终实现它，除非实现可以保证引用计数永远不会达到零（例如，对于静态分配的类型对象）。

NB: the methods for certain type groups are now contained in separate
method blocks.

注意：某些类型组的方法现在包含在单独的方法块中。
PySequenceMethods\PyNumberMethods\PyMappingMethods...

*/

typedef PyObject * (*unaryfunc)(PyObject *);
typedef PyObject * (*binaryfunc)(PyObject *, PyObject *);
typedef PyObject * (*ternaryfunc)(PyObject *, PyObject *, PyObject *);
typedef int (*inquiry)(PyObject *);
typedef Py_ssize_t (*lenfunc)(PyObject *);
typedef PyObject *(*ssizeargfunc)(PyObject *, Py_ssize_t);
typedef PyObject *(*ssizessizeargfunc)(PyObject *, Py_ssize_t, Py_ssize_t);
typedef int(*ssizeobjargproc)(PyObject *, Py_ssize_t, PyObject *);
typedef int(*ssizessizeobjargproc)(PyObject *, Py_ssize_t, Py_ssize_t, PyObject *);
typedef int(*objobjargproc)(PyObject *, PyObject *, PyObject *);

#ifndef Py_LIMITED_API
/* 
buffer interface 
缓冲区
与 Python 解释器公开的大多部数据类型不同，bufferObject不是 PyObject 指针而是简单的 C 结构。
这使得它们可以非常简单地创建和复制。
*/
typedef struct bufferinfo {
    void *buf;            /* 指向由缓冲区字段描述的逻辑结构开始的指针。 */
    PyObject *obj;        /* owned reference */
    Py_ssize_t len;
    Py_ssize_t itemsize;  /* This is Py_ssize_t so it can be
                             pointed to by strides in simple case.*/
    int readonly;         /* 缓冲区是否为只读的指示器。 */
    int ndim;
    char *format;
    Py_ssize_t *shape;
    Py_ssize_t *strides;
    Py_ssize_t *suboffsets;
    void *internal;
} Py_buffer;

typedef int (*getbufferproc)(PyObject *, Py_buffer *, int);
typedef void (*releasebufferproc)(PyObject *, Py_buffer *);

/* Maximum number of dimensions */
#define PyBUF_MAX_NDIM 64

/* Flags for getting buffers */
#define PyBUF_SIMPLE 0
#define PyBUF_WRITABLE 0x0001
/*  we used to include an E, backwards compatible alias  */
#define PyBUF_WRITEABLE PyBUF_WRITABLE
#define PyBUF_FORMAT 0x0004
#define PyBUF_ND 0x0008
#define PyBUF_STRIDES (0x0010 | PyBUF_ND)
#define PyBUF_C_CONTIGUOUS (0x0020 | PyBUF_STRIDES)
#define PyBUF_F_CONTIGUOUS (0x0040 | PyBUF_STRIDES)
#define PyBUF_ANY_CONTIGUOUS (0x0080 | PyBUF_STRIDES)
#define PyBUF_INDIRECT (0x0100 | PyBUF_STRIDES)

#define PyBUF_CONTIG (PyBUF_ND | PyBUF_WRITABLE)
#define PyBUF_CONTIG_RO (PyBUF_ND)

#define PyBUF_STRIDED (PyBUF_STRIDES | PyBUF_WRITABLE)
#define PyBUF_STRIDED_RO (PyBUF_STRIDES)

#define PyBUF_RECORDS (PyBUF_STRIDES | PyBUF_WRITABLE | PyBUF_FORMAT)
#define PyBUF_RECORDS_RO (PyBUF_STRIDES | PyBUF_FORMAT)

#define PyBUF_FULL (PyBUF_INDIRECT | PyBUF_WRITABLE | PyBUF_FORMAT)
#define PyBUF_FULL_RO (PyBUF_INDIRECT | PyBUF_FORMAT)


#define PyBUF_READ  0x100
#define PyBUF_WRITE 0x200

/* End buffer interface */
#endif /* Py_LIMITED_API */

typedef int (*objobjproc)(PyObject *, PyObject *);
typedef int (*visitproc)(PyObject *, void *);
typedef int (*traverseproc)(PyObject *, visitproc, void *);

#ifndef Py_LIMITED_API
typedef struct {
    /* Number implementations must check *both*
       arguments for proper type and implement the necessary conversions
       in the slot functions themselves. 
	  
       Number类型实现必须检查*两个*参数的正确类型，并在slot函数本身中实现必要的转换。
	*/

    binaryfunc nb_add;
    binaryfunc nb_subtract;
    binaryfunc nb_multiply;
    binaryfunc nb_remainder;
    binaryfunc nb_divmod;
    ternaryfunc nb_power;
    unaryfunc nb_negative;
    unaryfunc nb_positive;
    unaryfunc nb_absolute;
    inquiry nb_bool;
    unaryfunc nb_invert;
    binaryfunc nb_lshift;
    binaryfunc nb_rshift;
    binaryfunc nb_and;
    binaryfunc nb_xor;
    binaryfunc nb_or;
    unaryfunc nb_int;
    void *nb_reserved;  /* the slot formerly known as nb_long */
    unaryfunc nb_float;

    binaryfunc nb_inplace_add;
    binaryfunc nb_inplace_subtract;
    binaryfunc nb_inplace_multiply;
    binaryfunc nb_inplace_remainder;
    ternaryfunc nb_inplace_power;
    binaryfunc nb_inplace_lshift;
    binaryfunc nb_inplace_rshift;
    binaryfunc nb_inplace_and;
    binaryfunc nb_inplace_xor;
    binaryfunc nb_inplace_or;

    binaryfunc nb_floor_divide;
    binaryfunc nb_true_divide;
    binaryfunc nb_inplace_floor_divide;
    binaryfunc nb_inplace_true_divide;

    unaryfunc nb_index;

    binaryfunc nb_matrix_multiply;
    binaryfunc nb_inplace_matrix_multiply;
} PyNumberMethods;

typedef struct {
    lenfunc sq_length;
    binaryfunc sq_concat;
    ssizeargfunc sq_repeat;
    ssizeargfunc sq_item;
    void *was_sq_slice;
    ssizeobjargproc sq_ass_item;
    void *was_sq_ass_slice;
    objobjproc sq_contains;

    binaryfunc sq_inplace_concat;
    ssizeargfunc sq_inplace_repeat;
} PySequenceMethods;

typedef struct {
    lenfunc mp_length;
    binaryfunc mp_subscript;
    objobjargproc mp_ass_subscript;
} PyMappingMethods;

typedef struct {
    unaryfunc am_await;
    unaryfunc am_aiter;
    unaryfunc am_anext;
} PyAsyncMethods;

typedef struct {
     getbufferproc bf_getbuffer;
     releasebufferproc bf_releasebuffer;
} PyBufferProcs;
#endif /* Py_LIMITED_API */

typedef void (*freefunc)(void *);
typedef void (*destructor)(PyObject *);
#ifndef Py_LIMITED_API
/* We can't provide a full compile-time check that limited-API
   users won't implement tp_print. However, not defining printfunc
   and making tp_print of a different function pointer type
   should at least cause a warning in most cases. 
   
   我们无法提供完整的编译时检查，以确保有限的API用户不会实现tp_print。
   但是，在大多数情况下，不定义printfunc和使用不同的函数指针类型进行tp_print至少会引起警告。
 */
typedef int (*printfunc)(PyObject *, FILE *, int);
#endif
typedef PyObject *(*getattrfunc)(PyObject *, char *);
typedef PyObject *(*getattrofunc)(PyObject *, PyObject *);
typedef int (*setattrfunc)(PyObject *, char *, PyObject *);
typedef int (*setattrofunc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*reprfunc)(PyObject *);
typedef Py_hash_t (*hashfunc)(PyObject *);
typedef PyObject *(*richcmpfunc) (PyObject *, PyObject *, int);
typedef PyObject *(*getiterfunc) (PyObject *);
typedef PyObject *(*iternextfunc) (PyObject *);
typedef PyObject *(*descrgetfunc) (PyObject *, PyObject *, PyObject *);
typedef int (*descrsetfunc) (PyObject *, PyObject *, PyObject *);
typedef int (*initproc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*newfunc)(struct _typeobject *, PyObject *, PyObject *);
typedef PyObject *(*allocfunc)(struct _typeobject *, Py_ssize_t);

#ifdef Py_LIMITED_API
typedef struct _typeobject PyTypeObject; /* opaque */
#else
typedef struct _typeobject {
    PyObject_VAR_HEAD
    const char *tp_name; /* For printing, in format "<module>.<name>" */
    Py_ssize_t tp_basicsize, tp_itemsize; /* For allocation */

    /* Methods to implement standard operations */

    destructor tp_dealloc;
    printfunc tp_print;
    getattrfunc tp_getattr;      /* 3.8+弃用，改成tp_getattro */
    setattrfunc tp_setattr;      /* 3.8+弃用，改成tp_setattro */
    PyAsyncMethods *tp_as_async; /* formerly known as tp_compare (Python 2)
                                    or tp_reserved (Python 3) */
    reprfunc tp_repr;

    /* Method suites for standard classes */

    PyNumberMethods *tp_as_number;
    PySequenceMethods *tp_as_sequence;
    PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    hashfunc tp_hash;
    ternaryfunc tp_call;
    reprfunc tp_str;
    getattrofunc tp_getattro;
    setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    unsigned long tp_flags;

    const char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    traverseproc tp_traverse;

    /* delete references to contained objects */
    inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    richcmpfunc tp_richcompare;

    /* weak reference enabler */
    Py_ssize_t tp_weaklistoffset;

    /* Iterators */
    getiterfunc tp_iter;
    iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    struct PyMethodDef *tp_methods;
    struct PyMemberDef *tp_members;
    struct PyGetSetDef *tp_getset;
    struct _typeobject *tp_base;
    PyObject *tp_dict;
    descrgetfunc tp_descr_get;
    descrsetfunc tp_descr_set;
    Py_ssize_t tp_dictoffset;
    initproc tp_init;
    allocfunc tp_alloc;
    newfunc tp_new;
    freefunc tp_free; /* Low-level free-memory routine */
    inquiry tp_is_gc; /* For PyObject_IS_GC */
    PyObject *tp_bases;
    PyObject *tp_mro; /* method resolution order */
    PyObject *tp_cache;
    PyObject *tp_subclasses;
    PyObject *tp_weaklist;
    destructor tp_del;

    /* Type attribute cache version tag. Added in version 2.6 */
    unsigned int tp_version_tag;

    destructor tp_finalize;

#ifdef COUNT_ALLOCS
    /* these must be last and never explicitly initialized */
    Py_ssize_t tp_allocs;
    Py_ssize_t tp_frees;
    Py_ssize_t tp_maxalloc;
    struct _typeobject *tp_prev;
    struct _typeobject *tp_next;
#endif
} PyTypeObject;
#endif

typedef struct{
    int slot;    /* slot id, see below */
    void *pfunc; /* function pointer */
} PyType_Slot;

typedef struct{
    const char* name;
    int basicsize;
    int itemsize;
    unsigned int flags;
    PyType_Slot *slots; /* terminated by slot==0. */
} PyType_Spec;

PyAPI_FUNC(PyObject*) PyType_FromSpec(PyType_Spec*);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(PyObject*) PyType_FromSpecWithBases(PyType_Spec*, PyObject*);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03040000
PyAPI_FUNC(void*) PyType_GetSlot(PyTypeObject*, int);
#endif

#ifndef Py_LIMITED_API
/* 
The *real* layout of a type object when allocated on the heap 
在堆上分配时类型对象的*真实*布局
*/
typedef struct _heaptypeobject {
    /* Note: there's a dependency on the order of these members
       in slotptr() in typeobject.c . 
	  
       注意：typeobject.c中的slotptr（）中存在对这些成员顺序的依赖  
	   */
    PyTypeObject ht_type;
    PyAsyncMethods as_async;
    PyNumberMethods as_number;
    PyMappingMethods as_mapping;
    PySequenceMethods as_sequence; /* as_sequence comes after as_mapping,
                                      so that the mapping wins when both
                                      the mapping and the sequence define
                                      a given operator (e.g. __getitem__).
                                      see add_operators() in typeobject.c . 
									  
									  as_sequence位于as_mapping之后，因此当mapping和sequence都定义了给定的运算符
									  （例如__getitem__）时，mapping将获胜，即使用mapping的__getitem__方法。
									  请参见typeobject.c中的add_operators()。
									  
									  */
    PyBufferProcs as_buffer;
    PyObject *ht_name, *ht_slots, *ht_qualname;
    struct _dictkeysobject *ht_cached_keys;
    /* here are optional user slots, followed by the members. */
} PyHeapTypeObject;

/* access macro to the members which are floating "behind" the object */
#define PyHeapType_GET_MEMBERS(etype) \
    ((PyMemberDef *)(((char *)etype) + Py_TYPE(etype)->tp_basicsize))
#endif

/* Generic type check */
PyAPI_FUNC(int) PyType_IsSubtype(PyTypeObject *, PyTypeObject *);
#define PyObject_TypeCheck(ob, tp) \
    (Py_TYPE(ob) == (tp) || PyType_IsSubtype(Py_TYPE(ob), (tp)))

PyAPI_DATA(PyTypeObject) PyType_Type; /* built-in 'type' */
PyAPI_DATA(PyTypeObject) PyBaseObject_Type; /* built-in 'object' */
PyAPI_DATA(PyTypeObject) PySuper_Type; /* built-in 'super' */

PyAPI_FUNC(unsigned long) PyType_GetFlags(PyTypeObject*);

#define PyType_Check(op) \
    PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_TYPE_SUBCLASS)
#define PyType_CheckExact(op) (Py_TYPE(op) == &PyType_Type)

PyAPI_FUNC(int) PyType_Ready(PyTypeObject *);
PyAPI_FUNC(PyObject *) PyType_GenericAlloc(PyTypeObject *, Py_ssize_t);
PyAPI_FUNC(PyObject *) PyType_GenericNew(PyTypeObject *,
                                               PyObject *, PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(const char *) _PyType_Name(PyTypeObject *);
PyAPI_FUNC(PyObject *) _PyType_Lookup(PyTypeObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyType_LookupId(PyTypeObject *, _Py_Identifier *);
PyAPI_FUNC(PyObject *) _PyObject_LookupSpecial(PyObject *, _Py_Identifier *);
PyAPI_FUNC(PyTypeObject *) _PyType_CalculateMetaclass(PyTypeObject *, PyObject *);
#endif
PyAPI_FUNC(unsigned int) PyType_ClearCache(void);
PyAPI_FUNC(void) PyType_Modified(PyTypeObject *);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyType_GetDocFromInternalDoc(const char *, const char *);
PyAPI_FUNC(PyObject *) _PyType_GetTextSignatureFromInternalDoc(const char *, const char *);
#endif

/* Generic operations on objects */
#ifndef Py_LIMITED_API
struct _Py_Identifier;
PyAPI_FUNC(int) PyObject_Print(PyObject *, FILE *, int);
PyAPI_FUNC(void) _Py_BreakPoint(void);
PyAPI_FUNC(void) _PyObject_Dump(PyObject *);
PyAPI_FUNC(int) _PyObject_IsFreed(PyObject *);
#endif
PyAPI_FUNC(PyObject *) PyObject_Repr(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_Str(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_ASCII(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_Bytes(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_RichCompare(PyObject *, PyObject *, int);
PyAPI_FUNC(int) PyObject_RichCompareBool(PyObject *, PyObject *, int);
PyAPI_FUNC(PyObject *) PyObject_GetAttrString(PyObject *, const char *);
PyAPI_FUNC(int) PyObject_SetAttrString(PyObject *, const char *, PyObject *);
PyAPI_FUNC(int) PyObject_HasAttrString(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyObject_GetAttr(PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_SetAttr(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_HasAttr(PyObject *, PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyObject_IsAbstract(PyObject *);
PyAPI_FUNC(PyObject *) _PyObject_GetAttrId(PyObject *, struct _Py_Identifier *);
PyAPI_FUNC(int) _PyObject_SetAttrId(PyObject *, struct _Py_Identifier *, PyObject *);
PyAPI_FUNC(int) _PyObject_HasAttrId(PyObject *, struct _Py_Identifier *);
/* Replacements of PyObject_GetAttr() and _PyObject_GetAttrId() which
   don't raise AttributeError.

   替换不引发AttributeError的PyObject_GetAttr（）和_PyObject_GetAttrId（）。

   Return 1 and set *result != NULL if an attribute is found.
   Return 0 and set *result == NULL if an attribute is not found;
   an AttributeError is silenced.
   Return -1 and set *result == NULL if an error other than AttributeError
   is raised.

   如果找到属性，返回1并设置 *result!=NULL
   如果未找到属性，则返回0并设置 *result==NULL；AttributeError 被沉默。
   如果引发AttributeError以外的错误，则返回-1并设置 *result==NULL。

*/
PyAPI_FUNC(int) _PyObject_LookupAttr(PyObject *, PyObject *, PyObject **);
PyAPI_FUNC(int) _PyObject_LookupAttrId(PyObject *, struct _Py_Identifier *, PyObject **);
PyAPI_FUNC(PyObject **) _PyObject_GetDictPtr(PyObject *);
#endif
PyAPI_FUNC(PyObject *) PyObject_SelfIter(PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyObject_NextNotImplemented(PyObject *);
#endif
PyAPI_FUNC(PyObject *) PyObject_GenericGetAttr(PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_GenericSetAttr(PyObject *,
                                              PyObject *, PyObject *);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(int) PyObject_GenericSetDict(PyObject *, PyObject *, void *);
#endif
PyAPI_FUNC(Py_hash_t) PyObject_Hash(PyObject *);
PyAPI_FUNC(Py_hash_t) PyObject_HashNotImplemented(PyObject *);
PyAPI_FUNC(int) PyObject_IsTrue(PyObject *);
PyAPI_FUNC(int) PyObject_Not(PyObject *);
PyAPI_FUNC(int) PyCallable_Check(PyObject *);

PyAPI_FUNC(void) PyObject_ClearWeakRefs(PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) PyObject_CallFinalizer(PyObject *);
PyAPI_FUNC(int) PyObject_CallFinalizerFromDealloc(PyObject *);
#endif

#ifndef Py_LIMITED_API
/* Same as PyObject_Generic{Get,Set}Attr, but passing the attributes
   dict as the last parameter. 
   与PyObject_Generic{Get，Set}Attr相同，但将属性dict作为最后一个参数传递。
   */
PyAPI_FUNC(PyObject *)
_PyObject_GenericGetAttrWithDict(PyObject *, PyObject *, PyObject *, int);
PyAPI_FUNC(int)
_PyObject_GenericSetAttrWithDict(PyObject *, PyObject *,
                                 PyObject *, PyObject *);
#endif /* !Py_LIMITED_API */

/* Helper to look up a builtin object */
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *)
_PyObject_GetBuiltin(const char *name);
#endif

/* PyObject_Dir(obj) acts like Python builtins.dir(obj), returning a
   list of strings.  PyObject_Dir(NULL) is like builtins.dir(),
   returning the names of the current locals.  In this case, if there are
   no current locals, NULL is returned, and PyErr_Occurred() is false.

   PyObject_Dir（obj）的行为类似于Python builtins.Dir（obj），返回字符串列表。
   PyObject_Dir（NULL）类似于 builtins.Dir（），返回当前本地人的名称。
   在这种情况下，如果没有当前局部变量，并且PyErr_occurrent（）为false， 返回NULL.
*/
PyAPI_FUNC(PyObject *) PyObject_Dir(PyObject *);


/* Helpers for printing recursive container types */
PyAPI_FUNC(int) Py_ReprEnter(PyObject *);
PyAPI_FUNC(void) Py_ReprLeave(PyObject *);

/* Flag bits for printing: */
#define Py_PRINT_RAW    1       /* No string quotes etc. */

/*
`Type flags (tp_flags)
 类型标志

These flags are used to extend the type structure in a backwards-compatible
fashion. Extensions can use the flags to indicate (and test) when a given
type structure contains a new feature. The Python core will use these when
introducing new functionality between major revisions (to avoid mid-version
changes in the PYTHON_API_VERSION).

这些标志用于以向后兼容的方式扩展类型结构.
扩展可以使用标志指示（和测试）给定的包含一个新功能的类型结构。
Python核心将在主要版本之间引入新功能时使用这些标志（以避免PYTHON_API_VERSION中mid-version的更改）。


Arbitration of the flag bit positions will need to be coordinated among
all extension writers who publicly release their extensions (this will
be fewer than you might expect!)..

仲裁的标志位位置会被用来协调所有公开发布他们的扩展的扩展作者(这将比你想象的要少!).

Most flags were removed as of Python 3.0 to make room for new flags.  (Some
flags are not for backwards compatibility but to indicate the presence of an
optional feature; these flags remain of course.)

大多数标记在Python 3.0时被删除，以便为新标记腾出空间。
(一些标记不是为了向后兼容，而是指示存在可选特性;当然，这些标记还在。)

Type definitions should use Py_TPFLAGS_DEFAULT for their tp_flags value.

Code can use PyType_HasFeature(type_ob, flag_value) to test whether the
given type object has a specified feature.

类型定义的tp_flags值应使用Py_TPFLAGS_DEFAULT。

代码可以使用PyType_HasFeature(type_ob, flag_value)来测试给定类型对象是否具有指定的功能。
*/

/* Set if the type object is dynamically allocated */
#define Py_TPFLAGS_HEAPTYPE (1UL << 9)

/* Set if the type allows subclassing */
#define Py_TPFLAGS_BASETYPE (1UL << 10)

/* Set if the type is 'ready' -- fully initialized */
#define Py_TPFLAGS_READY (1UL << 12)

/* Set while the type is being 'readied', to prevent recursive ready calls */
#define Py_TPFLAGS_READYING (1UL << 13)

/* Objects support garbage collection (see objimp.h) */
#define Py_TPFLAGS_HAVE_GC (1UL << 14)

/* These two bits are preserved for Stackless Python, next after this is 17 */
#ifdef STACKLESS
#define Py_TPFLAGS_HAVE_STACKLESS_EXTENSION (3UL << 15)
#else
#define Py_TPFLAGS_HAVE_STACKLESS_EXTENSION 0
#endif

/* Objects support type attribute cache */
#define Py_TPFLAGS_HAVE_VERSION_TAG   (1UL << 18)
#define Py_TPFLAGS_VALID_VERSION_TAG  (1UL << 19)

/* Type is abstract and cannot be instantiated */
#define Py_TPFLAGS_IS_ABSTRACT (1UL << 20)

/* These flags are used to determine if a type is a subclass. */
#define Py_TPFLAGS_LONG_SUBCLASS        (1UL << 24)
#define Py_TPFLAGS_LIST_SUBCLASS        (1UL << 25)
#define Py_TPFLAGS_TUPLE_SUBCLASS       (1UL << 26)
#define Py_TPFLAGS_BYTES_SUBCLASS       (1UL << 27)
#define Py_TPFLAGS_UNICODE_SUBCLASS     (1UL << 28)
#define Py_TPFLAGS_DICT_SUBCLASS        (1UL << 29)
#define Py_TPFLAGS_BASE_EXC_SUBCLASS    (1UL << 30)
#define Py_TPFLAGS_TYPE_SUBCLASS        (1UL << 31)

#define Py_TPFLAGS_DEFAULT  ( \
                 Py_TPFLAGS_HAVE_STACKLESS_EXTENSION | \
                 Py_TPFLAGS_HAVE_VERSION_TAG | \
                0)

/* NOTE: The following flags reuse lower bits (removed as part of the
 * Python 3.0 transition). */

/* Type structure has tp_finalize member (3.4) */
#define Py_TPFLAGS_HAVE_FINALIZE (1UL << 0)

#ifdef Py_LIMITED_API
#define PyType_HasFeature(t,f)  ((PyType_GetFlags(t) & (f)) != 0)
#else
#define PyType_HasFeature(t,f)  (((t)->tp_flags & (f)) != 0)
#endif
#define PyType_FastSubclass(t,f)  PyType_HasFeature(t,f)


/*
The macros Py_INCREF(op) and Py_DECREF(op) are used to increment or decrement
reference counts.  Py_DECREF calls the object's deallocator function when
the refcount falls to 0; for
objects that don't contain references to other objects or heap memory
this can be the standard function free().  Both macros can be used
wherever a void expression is allowed.  The argument must not be a
NULL pointer.  If it may be NULL, use Py_XINCREF/Py_XDECREF instead.
The macro _Py_NewReference(op) initialize reference counts to 1, and
in special builds (Py_REF_DEBUG, Py_TRACE_REFS) performs additional
bookkeeping appropriate to the special build.

Py_INCREF(op)和Py_DECREF(op)宏用于递增或递减参考计数。
Py_DECREF在以下情况下调用对象的deallocator函数：
  refcount降为0；
对于不包含对其他对象或堆内存的引用的对象，这可以是标准的函数free（）。
两个宏都在任何允许空表达式的位置使用。
这个参数不能是一个错误空指针。如果它可能为空，请改用Py_XINCREF/Py_XDECREF。
宏_Py_NewReference（op）将引用计数初始化为1，
然后在特殊构建（Py_REF_DEBUG，Py_TRACE_REFS）中，执行额外的适合特殊构造的簿记。

We assume that the reference count field can never overflow; this can
be proven when the size of the field is the same as the pointer size, so
we ignore the possibility.  Provided a C int is at least 32 bits (which
is implicitly assumed in many parts of this code), that's enough for
about 2**31 references to an object.

我们假设引用计数域永远不会溢出;这可以在字段的大小与指针的大小相同时证明，所以我们忽略了这种可能性。
如果C int至少是32位(这在这段代码的许多部分中隐含地假定了)，这就足够了因为一个对象大约有2**31个引用。

XXX The following became out of date in Python 2.2, but I'm not sure
XXX what the full truth is now.  Certainly, heap-allocated type objects
XXX can and should be deallocated.
Type objects should never be deallocated; the type pointer in an object
is not considered to be a reference to the type object, to save
complications in the deallocation function.  (This is actually a
decision that's up to the implementer of each new type so if you want,
you can count such references to the type object.)

以下内容在Python 2.2中已经过时，但我不确定现在的全部情况是什么。
当然，堆分配的类型对象可以并且应该被释放。
类型对象永远不应该被释放;
对象中的类型指针不认为是对类型对象的引用，来解决回收功能的复杂性。
(这实际上是取决于每个新类型的实现者，所以如果你想，您可以计数对类型对象的此类引用。)

*/

/* First define a pile of simple helper macros, one set per special
 * build symbol.  These either expand to the obvious things, or to
 * nothing at all when the special mode isn't in effect.  The main
 * macros can later be defined just once then, yet expand to different
 * things depending on which special build options are and aren't in effect.
 * Trust me <wink>:  while painful, this is 20x easier to understand than,
 * e.g, defining _Py_NewReference five different times in a maze of nested
 * #ifdefs (we used to do that -- it was impenetrable).

 首先定义一堆简单的辅助宏，每个特殊宏一组建造符号。
 这些宏会扩展到显而易见的事情，或者扩展到nothing当特殊模式无效时。
 主宏可以迟些定义不过只能一次，但可以扩展为不同的宏，这取决于特殊构建选项有效或无效。
 相信我<wink>：虽然痛苦，但这比，例如，在一个迷宫式的嵌套结构中定义五次_Py_NewReference，理解起来要简单20倍
 #ifdefs（我们过去经常这样做——它是无法穿透的）。

 */
#ifdef Py_REF_DEBUG
PyAPI_DATA(Py_ssize_t) _Py_RefTotal;
PyAPI_FUNC(void) _Py_NegativeRefcount(const char *fname,
                                            int lineno, PyObject *op);
PyAPI_FUNC(Py_ssize_t) _Py_GetRefTotal(void);
#define _Py_INC_REFTOTAL        _Py_RefTotal++
#define _Py_DEC_REFTOTAL        _Py_RefTotal--
#define _Py_REF_DEBUG_COMMA     ,
#define _Py_CHECK_REFCNT(OP)                                    \
{       if (((PyObject*)OP)->ob_refcnt < 0)                             \
                _Py_NegativeRefcount(__FILE__, __LINE__,        \
                                     (PyObject *)(OP));         \
}
/* Py_REF_DEBUG also controls the display of refcounts and memory block
 * allocations at the interactive prompt and at interpreter shutdown
 */
PyAPI_FUNC(void) _PyDebug_PrintTotalRefs(void);
#else
#define _Py_INC_REFTOTAL
#define _Py_DEC_REFTOTAL
#define _Py_REF_DEBUG_COMMA
#define _Py_CHECK_REFCNT(OP)    /* a semicolon */;
#endif /* Py_REF_DEBUG */

#ifdef COUNT_ALLOCS
PyAPI_FUNC(void) inc_count(PyTypeObject *);
PyAPI_FUNC(void) dec_count(PyTypeObject *);
#define _Py_INC_TPALLOCS(OP)    inc_count(Py_TYPE(OP))
#define _Py_INC_TPFREES(OP)     dec_count(Py_TYPE(OP))
#define _Py_DEC_TPFREES(OP)     Py_TYPE(OP)->tp_frees--
#define _Py_COUNT_ALLOCS_COMMA  ,
#else
#define _Py_INC_TPALLOCS(OP)
#define _Py_INC_TPFREES(OP)
#define _Py_DEC_TPFREES(OP)
#define _Py_COUNT_ALLOCS_COMMA
#endif /* COUNT_ALLOCS */

#ifdef Py_TRACE_REFS
/* Py_TRACE_REFS is such major surgery that we call external routines. */
PyAPI_FUNC(void) _Py_NewReference(PyObject *);
PyAPI_FUNC(void) _Py_ForgetReference(PyObject *);
PyAPI_FUNC(void) _Py_Dealloc(PyObject *);
PyAPI_FUNC(void) _Py_PrintReferences(FILE *);
PyAPI_FUNC(void) _Py_PrintReferenceAddresses(FILE *);
PyAPI_FUNC(void) _Py_AddToAllObjects(PyObject *, int force);

#else
/* Without Py_TRACE_REFS, there's little enough to do that we expand code
 * inline.
 */
#define _Py_NewReference(op) (                          \
    _Py_INC_TPALLOCS(op) _Py_COUNT_ALLOCS_COMMA         \
    _Py_INC_REFTOTAL  _Py_REF_DEBUG_COMMA               \
    Py_REFCNT(op) = 1)

#define _Py_ForgetReference(op) _Py_INC_TPFREES(op)

#ifdef Py_LIMITED_API
PyAPI_FUNC(void) _Py_Dealloc(PyObject *);
#else
#define _Py_Dealloc(op) (                               \
    _Py_INC_TPFREES(op) _Py_COUNT_ALLOCS_COMMA          \
    (*Py_TYPE(op)->tp_dealloc)((PyObject *)(op)))
#endif
#endif /* !Py_TRACE_REFS */

#define Py_INCREF(op) (                         \
    _Py_INC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
    ((PyObject *)(op))->ob_refcnt++)

#define Py_DECREF(op)                                   \
    do {                                                \
        PyObject *_py_decref_tmp = (PyObject *)(op);    \
        if (_Py_DEC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
        --(_py_decref_tmp)->ob_refcnt != 0)             \
            _Py_CHECK_REFCNT(_py_decref_tmp)            \
        else                                            \
            _Py_Dealloc(_py_decref_tmp);                \
    } while (0)

/* Safely decref `op` and set `op` to NULL, especially useful in tp_clear
 * and tp_dealloc implementations.
 *
 * Note that "the obvious" code can be deadly:
 *
 *     Py_XDECREF(op);
 *     op = NULL;
 *
 * Typically, `op` is something like self->containee, and `self` is done
 * using its `containee` member.  In the code sequence above, suppose
 * `containee` is non-NULL with a refcount of 1.  Its refcount falls to
 * 0 on the first line, which can trigger an arbitrary amount of code,
 * possibly including finalizers (like __del__ methods or weakref callbacks)
 * coded in Python, which in turn can release the GIL and allow other threads
 * to run, etc.  Such code may even invoke methods of `self` again, or cause
 * cyclic gc to trigger, but-- oops! --self->containee still points to the
 * object being torn down, and it may be in an insane state while being torn
 * down.  This has in fact been a rich historic source of miserable (rare &
 * hard-to-diagnose) segfaulting (and other) bugs.
 *

 安全地减少'op'并将'op'设置为NULL，在tp_clear和tp_dealoc的实现中尤其有用。
 注意：明显的代码可能会是致命的：
     Py_XDECREF(op);
     op = NULL;
 通常，`op`类似于self->containee，`self`是用它的“containee”成员完成的。
 在上面的代码序列中，假设containee'非NULL，引用计数为1。其参考数量在第一行下降为0，
 它可以触发任意数量的代码，可能包括终结器（如__del__方法或weakref回调）
 这反过来可以释放GIL并允许其他线程运行。
 这类代码甚至可能再次调用“self”的方法，或导致循环gc触发。

 * The safe way is:
 *
 *      Py_CLEAR(op);
 *
 * That arranges to set `op` to NULL _before_ decref'ing, so that any code
 * triggered as a side-effect of `op` getting torn down no longer believes
 * `op` points to a valid object.
 *
 * There are cases where it's safe to use the naive code, but they're brittle.
 * For example, if `op` points to a Python integer, you know that destroying
 * one of those can't cause problems -- but in part that relies on that
 * Python integers aren't currently weakly referencable.  Best practice is
 * to use Py_CLEAR() even if you can't think of a reason for why you need to.

 安全的方式是：
     Py_CLEAR(op);
 这种方式安排在递减之前将'op'设置为NULL'，这样任何代码由于“op”被拆毁的副作用而触发，
 不用再相信'op'指向一个有效的对象。
 
 有些情况下，使用朴素的代码是安全的，但它们是脆弱的。
 例如，如果'op'指向一个Python整数，你就知道摧毁其中之一不会引起问题，
 但部分原因在于此Python整数当前不可弱引用。
 最佳方式是使用Py_CLEAR（），即使你想不出需要使用Py_CLEAR（）的原因。

 */
#define Py_CLEAR(op)                            \
    do {                                        \
        PyObject *_py_tmp = (PyObject *)(op);   \
        if (_py_tmp != NULL) {                  \
            (op) = NULL;                        \
            Py_DECREF(_py_tmp);                 \
        }                                       \
    } while (0)

/* Macros to use in case the object pointer may be NULL: */
#define Py_XINCREF(op)                                \
    do {                                              \
        PyObject *_py_xincref_tmp = (PyObject *)(op); \
        if (_py_xincref_tmp != NULL)                  \
            Py_INCREF(_py_xincref_tmp);               \
    } while (0)

#define Py_XDECREF(op)                                \
    do {                                              \
        PyObject *_py_xdecref_tmp = (PyObject *)(op); \
        if (_py_xdecref_tmp != NULL)                  \
            Py_DECREF(_py_xdecref_tmp);               \
    } while (0)

#ifndef Py_LIMITED_API
/* Safely decref `op` and set `op` to `op2`.
   安全地取消'op'并将'op'设置为'op2'。
 *
 * As in case of Py_CLEAR "the obvious" code can be deadly:
   与Py_CLEAR的情况一样，“明显的”代码可能是致命的：
 *
 *     Py_DECREF(op);
 *     op = op2;
 *
 * The safe way is:
   安全的方式是：
 *
 *      Py_SETREF(op, op2);
 *
 * That arranges to set `op` to `op2` _before_ decref'ing, so that any code
 * triggered as a side-effect of `op` getting torn down no longer believes
 * `op` points to a valid object.
 *
   这种安排在递减之前将“op”设置为“op2”，以便任何代码可以因为“op”被拆下的副作用而触发，
   不用相信'op'指向一个有效的对象。

 * Py_XSETREF is a variant of Py_SETREF that uses Py_XDECREF instead of
 * Py_DECREF.
   Py_XSETREF是Py_SETREF的一个变体，它使用Py_XDECREF而不是Py_DECREF。

 */

#define Py_SETREF(op, op2)                      \
    do {                                        \
        PyObject *_py_tmp = (PyObject *)(op);   \
        (op) = (op2);                           \
        Py_DECREF(_py_tmp);                     \
    } while (0)

#define Py_XSETREF(op, op2)                     \
    do {                                        \
        PyObject *_py_tmp = (PyObject *)(op);   \
        (op) = (op2);                           \
        Py_XDECREF(_py_tmp);                    \
    } while (0)

#endif /* ifndef Py_LIMITED_API */

/*
These are provided as conveniences to Python runtime embedders, so that
they can have object code that is not dependent on Python compilation flags.

这些都是为Python运行时嵌入程序提供的便利，因此它们可以具有不依赖于Python编译标志的对象代码。
*/
PyAPI_FUNC(void) Py_IncRef(PyObject *);
PyAPI_FUNC(void) Py_DecRef(PyObject *);

#ifndef Py_LIMITED_API
PyAPI_DATA(PyTypeObject) _PyNone_Type;
PyAPI_DATA(PyTypeObject) _PyNotImplemented_Type;
#endif /* !Py_LIMITED_API */

/*
_Py_NoneStruct is an object of undefined type which can be used in contexts
where NULL (nil) is not suitable (since NULL often means 'error').

Don't forget to apply Py_INCREF() when returning this value!!!

_Py_NoneStruct是未定义类型的对象，可在上下文中使用
其中NULL（nil）不合适（因为NULL通常表示“错误”）。

返回此值时不要忘记应用Py_incremf（）！！！

*/
PyAPI_DATA(PyObject) _Py_NoneStruct; /* Don't use this directly */
#define Py_None (&_Py_NoneStruct)

/* Macro for returning Py_None from a function */
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None

/*
Py_NotImplemented is a singleton used to signal that an operation is
not implemented for a given type combination.

Py_NotImplemented是一个单例，用于表示操作对给定类型组合没有被实现。
*/
PyAPI_DATA(PyObject) _Py_NotImplementedStruct; /* Don't use this directly */
#define Py_NotImplemented (&_Py_NotImplementedStruct)

/* Macro for returning Py_NotImplemented from a function */
#define Py_RETURN_NOTIMPLEMENTED \
    return Py_INCREF(Py_NotImplemented), Py_NotImplemented

/* Rich comparison opcodes */
#define Py_LT 0
#define Py_LE 1
#define Py_EQ 2
#define Py_NE 3
#define Py_GT 4
#define Py_GE 5

/*
 * Macro for implementing rich comparisons
 *
 * Needs to be a macro because any C-comparable type can be used.
 */
#define Py_RETURN_RICHCOMPARE(val1, val2, op)                               \
    do {                                                                    \
        switch (op) {                                                       \
        case Py_EQ: if ((val1) == (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        case Py_NE: if ((val1) != (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        case Py_LT: if ((val1) < (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;   \
        case Py_GT: if ((val1) > (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;   \
        case Py_LE: if ((val1) <= (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        case Py_GE: if ((val1) >= (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        default:                                                            \
            Py_UNREACHABLE();                                               \
        }                                                                   \
    } while (0)

#ifndef Py_LIMITED_API
/* Maps Py_LT to Py_GT, ..., Py_GE to Py_LE.
 * Defined in object.c.
 */
PyAPI_DATA(int) _Py_SwappedOp[];
#endif /* !Py_LIMITED_API */


/*
More conventions
更多惯例
================

Argument Checking
参数检查
-----------------

Functions that take objects as arguments normally don't check for nil
arguments, but they do check the type of the argument, and return an
error if the function doesn't apply to the type.

将对象作为参数的函数通常不检查nil参数，但它们会检查参数的类型，并在函数不应用于类型时返回错误。

Failure Modes
失效模式
-------------

Functions may fail for a variety of reasons, including running out of
memory.  This is communicated to the caller in two ways: an error string
is set (see errors.h), and the function result differs: functions that
normally return a pointer return NULL for failure, functions returning
an integer return -1 (which could be a legal return value too!), and
other functions return 0 for success and -1 for failure.
Callers should always check for errors before using the result.  If
an error was set, the caller must either explicitly clear it, or pass
the error on to its caller.

函数可能由于各种原因而失败，包括耗尽内存空间。
这通过两种方式与调用者通信：
错误字符串设置（请参见errors.h），
以及函数结果不同：函数通常返回一个指针，失败时返回NULL，
函数返回整数返回值-1（也可能是合法的返回值！），以及其他函数返回0表示成功，返回-1表示失败。
调用方在使用结果之前应始终检查错误。如果设置了错误，调用方必须明确清除该错误，或传递该错误将传递给其调用方。

Reference Counts
引用计数
----------------

It takes a while to get used to the proper usage of reference counts.
需要一段时间才能习惯引用计数的正确用法。

Functions that create an object set the reference count to 1; such new
objects must be stored somewhere or destroyed again with Py_DECREF().
Some functions that 'store' objects, such as PyTuple_SetItem() and
PyList_SetItem(),
don't increment the reference count of the object, since the most
frequent use is to store a fresh object.  Functions that 'retrieve'
objects, such as PyTuple_GetItem() and PyDict_GetItemString(), also
don't increment
the reference count, since most frequently the object is only looked at
quickly.  Thus, to retrieve an object and store it again, the caller
must call Py_INCREF() explicitly.

创建对象的函数将引用计数设置为1；如此新对象必须使用Py_DECREF（）将对象存储在某处或再次销毁。
一些“存储”对象的函数，如PyTuple_SetItem（）和PyList_SetItem（），
不会增加对象的引用计数，因为经常使用的是存储一个新的对象。
“检索”对象的函数，例如PyTuple_GetItem（）和PyDict_GetItemString（）
不会增加引用计数，因为最常见的情况是只迅速地查看对象。
因此，要检索对象并再次存储它，调用方必须显式调用Py_INCREF（）。

NOTE: functions that 'consume' a reference count, like
PyList_SetItem(), consume the reference even if the object wasn't
successfully stored, to simplify error handling.

注意：“使用”引用计数的函数，如PyList_SetItem（），即使对象没有成功存储，要简化错误处理。

It seems attractive to make other functions that take an object as
argument consume a reference count; however, this may quickly get
confusing (even the current practice is already confusing).  Consider
it carefully, it may save lots of calls to Py_INCREF() and Py_DECREF() at
times.

制作以对象为参数消耗引用计数的其他函数似乎很有吸引力；然而，这可能会很快令人困惑（即使是目前的做法也已经令人困惑）。
仔细考虑一下，它可能会节省大量对Py_INCREF（）和Py_DECREF（）的调用。

*/


/* Trashcan mechanism, thanks to Christian Tismer.
   垃圾桶机制
When deallocating a container object, it's possible to trigger an unbounded
chain of deallocations, as each Py_DECREF in turn drops the refcount on "the
next" object in the chain to 0.  This can easily lead to stack faults, and
especially in threads (which typically have less stack space to work with).

释放容器对象时，可能会触发一个无界的回收链，每个Py_DECREF依次将链中的“下一个”对象设置为0。
这很容易导致堆栈故障，特别是在线程中（通常具有较少的堆栈空间）。

A container object that participates in cyclic gc can avoid this by
bracketing the body of its tp_dealloc function with a pair of macros:

参与循环gc的容器对象可以通过用一对宏将其tp_dealloc函数体括起来来避免这个问题：
如下：

static void
mytype_dealloc(mytype *p)
{
    ... declarations go here ...

    PyObject_GC_UnTrack(p);        // must untrack first
    Py_TRASHCAN_SAFE_BEGIN(p)
    ... The body of the deallocator goes here, including all calls ...
    ... to Py_DECREF on contained objects.                         ...
    Py_TRASHCAN_SAFE_END(p)
}

CAUTION:  Never return from the middle of the body!  If the body needs to
"get out early", put a label immediately before the Py_TRASHCAN_SAFE_END
call, and goto it.  Else the call-depth counter (see below) will stay
above 0 forever, and the trashcan will never get emptied.

注意：切勿从代码中部返回！如果主体需要“早点退出”，在Py_TRASHCAN_SAFE_END调用前贴上标签，然后开始。
否则，调用深度计数器（见下文）将保持永远高于0，垃圾桶永远不会被清空。

How it works:  The BEGIN macro increments a call-depth counter.  So long
as this counter is small, the body of the deallocator is run directly without
further ado.  But if the counter gets large, it instead adds p to a list of
objects to be deallocated later, skips the body of the deallocator, and
resumes execution after the END macro.  The tp_dealloc routine then returns
without deallocating anything (and so unbounded call-stack depth is avoided).

工作原理：BEGIN宏递增调用深度计数器。由于此计数器很小，deallocator的主体直接运行，而无需进一步的ado。
但是如果计数器变大，它会稍后将p添加到需要回收的对象列表，跳过deallocator的主体，然后在结束宏后恢复执行。
然后，tp_dealloc例程返回，无需取消分配任何内容（从而避免了无限调用堆栈深度）。

When the call stack finishes unwinding again, code generated by the END macro
notices this, and calls another routine to deallocate all the objects that
may have been added to the list of deferred deallocations.  In effect, a
chain of N deallocations is broken into (N-1)/(PyTrash_UNWIND_LEVEL-1) pieces,
with the call stack never exceeding a depth of PyTrash_UNWIND_LEVEL.

当调用堆栈再次完成展开时，END宏生成的代码会注意到这一点，并调用另一个例程来取消分配
那些可能已添加到延迟解除分配列表中的对象。实际上，一个N个回收链分为（N-1）/（PyTrash_UNWIND_LEVEL-1）个部分，
调用堆栈的深度永远不会超过PyTrash_UNWIND_LEVEL。

*/

#ifndef Py_LIMITED_API
/* This is the old private API, invoked by the macros before 3.2.4.
   Kept for binary compatibility of extensions using the stable ABI. */
PyAPI_FUNC(void) _PyTrash_deposit_object(PyObject*);
PyAPI_FUNC(void) _PyTrash_destroy_chain(void);
#endif /* !Py_LIMITED_API */

/* The new thread-safe private API, invoked by the macros below. */
PyAPI_FUNC(void) _PyTrash_thread_deposit_object(PyObject*);
PyAPI_FUNC(void) _PyTrash_thread_destroy_chain(void);

#define PyTrash_UNWIND_LEVEL 50

#define Py_TRASHCAN_SAFE_BEGIN(op) \
    do { \
        PyThreadState *_tstate = PyThreadState_GET(); \
        if (_tstate->trash_delete_nesting < PyTrash_UNWIND_LEVEL) { \
            ++_tstate->trash_delete_nesting;
            /* The body of the deallocator is here. */
#define Py_TRASHCAN_SAFE_END(op) \
            --_tstate->trash_delete_nesting; \
            if (_tstate->trash_delete_later && _tstate->trash_delete_nesting <= 0) \
                _PyTrash_thread_destroy_chain(); \
        } \
        else \
            _PyTrash_thread_deposit_object((PyObject*)op); \
    } while (0);

#ifndef Py_LIMITED_API
PyAPI_FUNC(void)
_PyDebugAllocatorStats(FILE *out, const char *block_name, int num_blocks,
                       size_t sizeof_block);
PyAPI_FUNC(void)
_PyObject_DebugTypeStats(FILE *out);
#endif /* ifndef Py_LIMITED_API */

#ifdef __cplusplus
}
#endif
#endif /* !Py_OBJECT_H */
