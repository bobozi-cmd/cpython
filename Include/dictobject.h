#ifndef Py_DICTOBJECT_H
#define Py_DICTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif
	/*
	* 条件指示符#ifndef 的最主要目的是防止头文件的重复包含和编译。
	#ifndef x                 //先测试x是否被宏定义过
	#define x
	   程序段1blabla~    //如果x没有被宏定义过，定义x，并编译程序段 1
	#endif
	　　程序段2blabla~　　 //如果x已经定义过了则编译程序段2的语句，“忽视”程序段 1
	extern "C"的主要作用就是为了能够正确实现C++代码调用其他C语言代码。加上extern "C"后，会指示编译器这部分代码按C语言（而不是C++）的方式进行编译。
	*/

/* 
Dictionary object type -- mapping from hashable object to object 
字典对象类型--从可散列对象到对象的映射
*/

/* The distribution includes a separate file, Objects/dictnotes.txt,
   describing explorations into dictionary design and optimization.
   It covers typical dictionary use patterns, the parameters for
   tuning dictionaries, and several ideas for possible optimizations.

   这个发行版包括一个单独的文件，Objects/dictnotes.txt,
   描述了对字典设计和优化的探索,它涵盖了典型的字典使用模式，
   用于调整字典的参数,以及一些可能的优化的想法。

*/

#ifndef Py_LIMITED_API

typedef struct _dictkeysobject PyDictKeysObject;

/* The ma_values pointer is NULL for a combined table
 * or points to an array of PyObject* for a split table

 对于一个合并的表，ma_values指针是NULL。或指向一个分割表的PyObject*数组。

 */
typedef struct {
    PyObject_HEAD

    /* 
	Number of items in the dictionary 
	字典中元素个数
	*/
    Py_ssize_t ma_used;

    /* Dictionary version: globally unique, value change each time
       the dictionary is modified 
	   
	   字典版本：全局唯一，每次修改字典的定义时值都会改变
	   */
    uint64_t ma_version_tag;

    PyDictKeysObject *ma_keys;

    /* If ma_values is NULL, the table is "combined": keys and values
       are stored in ma_keys.

       如果ma_values是NULL，那么该表是 "组合 "的：键和值都存储在ma_keys中。

       If ma_values is not NULL, the table is splitted:
       keys are stored in ma_keys and values are stored in ma_values 
	   
	   如果ma_values不是NULL，那么该表将被拆分。键被存储在ma_keys中，值被存储在ma_values中。

	   */
    PyObject **ma_values;
} PyDictObject;

typedef struct {
    PyObject_HEAD
    PyDictObject *dv_dict;
} _PyDictViewObject;

#endif /* Py_LIMITED_API */

PyAPI_DATA(PyTypeObject) PyDict_Type;
PyAPI_DATA(PyTypeObject) PyDictIterKey_Type;
PyAPI_DATA(PyTypeObject) PyDictIterValue_Type;
PyAPI_DATA(PyTypeObject) PyDictIterItem_Type;
PyAPI_DATA(PyTypeObject) PyDictKeys_Type;
PyAPI_DATA(PyTypeObject) PyDictItems_Type;
PyAPI_DATA(PyTypeObject) PyDictValues_Type;

#define PyDict_Check(op) \
                 PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_DICT_SUBCLASS)
#define PyDict_CheckExact(op) (Py_TYPE(op) == &PyDict_Type)
#define PyDictKeys_Check(op) PyObject_TypeCheck(op, &PyDictKeys_Type)
#define PyDictItems_Check(op) PyObject_TypeCheck(op, &PyDictItems_Type)
#define PyDictValues_Check(op) PyObject_TypeCheck(op, &PyDictValues_Type)
/* 
This excludes Values, since they are not sets. 
这不包含Values,因为它们是集合
*/
# define PyDictViewSet_Check(op) \
    (PyDictKeys_Check(op) || PyDictItems_Check(op))


PyAPI_FUNC(PyObject *) PyDict_New(void);
PyAPI_FUNC(PyObject *) PyDict_GetItem(PyObject *mp, PyObject *key);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyDict_GetItem_KnownHash(PyObject *mp, PyObject *key,
                                       Py_hash_t hash);
#endif
PyAPI_FUNC(PyObject *) PyDict_GetItemWithError(PyObject *mp, PyObject *key);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyDict_GetItemIdWithError(PyObject *dp,
                                                  struct _Py_Identifier *key);
PyAPI_FUNC(PyObject *) PyDict_SetDefault(
    PyObject *mp, PyObject *key, PyObject *defaultobj);
#endif
PyAPI_FUNC(int) PyDict_SetItem(PyObject *mp, PyObject *key, PyObject *item);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key,
                                          PyObject *item, Py_hash_t hash);
#endif
PyAPI_FUNC(int) PyDict_DelItem(PyObject *mp, PyObject *key);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyDict_DelItem_KnownHash(PyObject *mp, PyObject *key,
                                          Py_hash_t hash);
PyAPI_FUNC(int) _PyDict_DelItemIf(PyObject *mp, PyObject *key,
                                  int (*predicate)(PyObject *value));
#endif
PyAPI_FUNC(void) PyDict_Clear(PyObject *mp);
PyAPI_FUNC(int) PyDict_Next(
    PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value);
#ifndef Py_LIMITED_API
PyDictKeysObject *_PyDict_NewKeysForClass(void);
PyAPI_FUNC(PyObject *) PyObject_GenericGetDict(PyObject *, void *);
PyAPI_FUNC(int) _PyDict_Next(
    PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value, Py_hash_t *hash);
PyObject *_PyDictView_New(PyObject *, PyTypeObject *);
#endif
PyAPI_FUNC(PyObject *) PyDict_Keys(PyObject *mp);
PyAPI_FUNC(PyObject *) PyDict_Values(PyObject *mp);
PyAPI_FUNC(PyObject *) PyDict_Items(PyObject *mp);
PyAPI_FUNC(Py_ssize_t) PyDict_Size(PyObject *mp);
PyAPI_FUNC(PyObject *) PyDict_Copy(PyObject *mp);
PyAPI_FUNC(int) PyDict_Contains(PyObject *mp, PyObject *key);
#ifndef Py_LIMITED_API
/* 
Get the number of items of a dictionary. 
获得字典中元素个数
*/
#define PyDict_GET_SIZE(mp)  (assert(PyDict_Check(mp)),((PyDictObject *)mp)->ma_used)
PyAPI_FUNC(int) _PyDict_Contains(PyObject *mp, PyObject *key, Py_hash_t hash);
PyAPI_FUNC(PyObject *) _PyDict_NewPresized(Py_ssize_t minused);
PyAPI_FUNC(void) _PyDict_MaybeUntrack(PyObject *mp);
PyAPI_FUNC(int) _PyDict_HasOnlyStringKeys(PyObject *mp);
Py_ssize_t _PyDict_KeysSize(PyDictKeysObject *keys);
PyAPI_FUNC(Py_ssize_t) _PyDict_SizeOf(PyDictObject *);
PyAPI_FUNC(PyObject *) _PyDict_Pop(PyObject *, PyObject *, PyObject *);
PyObject *_PyDict_Pop_KnownHash(PyObject *, PyObject *, Py_hash_t, PyObject *);
PyObject *_PyDict_FromKeys(PyObject *, PyObject *, PyObject *);
#define _PyDict_HasSplitTable(d) ((d)->ma_values != NULL)

PyAPI_FUNC(int) PyDict_ClearFreeList(void);
#endif

/* 
PyDict_Update(mp, other) is equivalent to PyDict_Merge(mp, other, 1). 
PyDict_Update(mp, other)等价于PyDict_Merge(mp, other, 1)
*/
PyAPI_FUNC(int) PyDict_Update(PyObject *mp, PyObject *other);

/* PyDict_Merge updates/merges from a mapping object (an object that
   supports PyMapping_Keys() and PyObject_GetItem()).  If override is true,
   the last occurrence of a key wins, else the first.  The Python
   dict.update(other) is equivalent to PyDict_Merge(dict, other, 1).

   PyDict_Merge从一个映射对象（一个支持PyMapping_Keys()和PyObject_GetItem()的对象）更新/合并。
   如果override为true,最后出现的一个键获胜，否则是第一个。
   Python的dict.update(other) 相当于 PyDict_Merge(dict, other, 1)。
*/
PyAPI_FUNC(int) PyDict_Merge(PyObject *mp,
                                   PyObject *other,
                                   int override);

#ifndef Py_LIMITED_API
/* Like PyDict_Merge, but override can be 0, 1 or 2.  If override is 0,
   the first occurrence of a key wins, if override is 1, the last occurrence
   of a key wins, if override is 2, a KeyError with conflicting key as
   argument is raised.

   和PyDict_Merge一样，但override可以是0、1或2。 如果override是0。
   键的第一次出现为胜，如果覆盖为1，则键的最后一次出现为胜。
   如果override是2，会产生一个以冲突键为参数的KeyError。

*/
PyAPI_FUNC(int) _PyDict_MergeEx(PyObject *mp, PyObject *other, int override);
PyAPI_FUNC(PyObject *) _PyDictView_Intersect(PyObject* self, PyObject *other);
#endif

/* PyDict_MergeFromSeq2 updates/merges from an iterable object producing
   iterable objects of length 2.  If override is true, the last occurrence
   of a key wins, else the first.  The Python dict constructor dict(seq2)
   is equivalent to dict={}; PyDict_MergeFromSeq(dict, seq2, 1).

   PyDict_MergeFromSeq2从一个可迭代对象更新/合并产生长度为2的可迭代对象。
   如果覆盖为真，则最后出现的键获胜，否则为第一次出现的键获胜。
   Python的dict构造函数dict(seq2) 相当于dict={}; PyDict_MergeFromSeq(dict, seq2, 1)。

*/
PyAPI_FUNC(int) PyDict_MergeFromSeq2(PyObject *d,
                                           PyObject *seq2,
                                           int override);

PyAPI_FUNC(PyObject *) PyDict_GetItemString(PyObject *dp, const char *key);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyDict_GetItemId(PyObject *dp, struct _Py_Identifier *key);
#endif /* !Py_LIMITED_API */
PyAPI_FUNC(int) PyDict_SetItemString(PyObject *dp, const char *key, PyObject *item);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyDict_SetItemId(PyObject *dp, struct _Py_Identifier *key, PyObject *item);
#endif /* !Py_LIMITED_API */
PyAPI_FUNC(int) PyDict_DelItemString(PyObject *dp, const char *key);

#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyDict_DelItemId(PyObject *mp, struct _Py_Identifier *key);
PyAPI_FUNC(void) _PyDict_DebugMallocStats(FILE *out);

int _PyObjectDict_SetItem(PyTypeObject *tp, PyObject **dictptr, PyObject *name, PyObject *value);
PyObject *_PyDict_LoadGlobal(PyDictObject *, PyDictObject *, PyObject *);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_DICTOBJECT_H */
