#ifndef Py_LONGOBJECT_H
#define Py_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* 
Long (arbitrary precision) integer object interface 
长整数对象接口（任意精度）
*/

typedef struct _longobject PyLongObject; /* Revealed in longintrepr.h 在longintrepr.h中显示  */

PyAPI_DATA(PyTypeObject) PyLong_Type;

#define PyLong_Check(op) \
        PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_LONG_SUBCLASS)
#define PyLong_CheckExact(op) (Py_TYPE(op) == &PyLong_Type)

PyAPI_FUNC(PyObject *) PyLong_FromLong(long);
PyAPI_FUNC(PyObject *) PyLong_FromUnsignedLong(unsigned long);
PyAPI_FUNC(PyObject *) PyLong_FromSize_t(size_t);
PyAPI_FUNC(PyObject *) PyLong_FromSsize_t(Py_ssize_t);
PyAPI_FUNC(PyObject *) PyLong_FromDouble(double);
PyAPI_FUNC(long) PyLong_AsLong(PyObject *);
PyAPI_FUNC(long) PyLong_AsLongAndOverflow(PyObject *, int *);
PyAPI_FUNC(Py_ssize_t) PyLong_AsSsize_t(PyObject *);
PyAPI_FUNC(size_t) PyLong_AsSize_t(PyObject *);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLong(PyObject *);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLongMask(PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyLong_AsInt(PyObject *);
#endif
PyAPI_FUNC(PyObject *) PyLong_GetInfo(void);

/* It may be useful in the future. I've added it in the PyInt -> PyLong
   cleanup to keep the extra information. [CH] 
   
   在将来这或许会变得有用。我在PyInt->PyLong转换中添加了它，以保留额外的信息。

   */
#define PyLong_AS_LONG(op) PyLong_AsLong(op)

/* Issue #1983: pid_t can be longer than a C long on some systems 
   问题 #1983：在某些系统中pid_t的长度可能超过C的long
*/
#if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
#define _Py_PARSE_PID "i"
#define PyLong_FromPid PyLong_FromLong
#define PyLong_AsPid PyLong_AsLong
#elif SIZEOF_PID_T == SIZEOF_LONG
#define _Py_PARSE_PID "l"
#define PyLong_FromPid PyLong_FromLong
#define PyLong_AsPid PyLong_AsLong
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_PID_T == SIZEOF_LONG_LONG
#define _Py_PARSE_PID "L"
#define PyLong_FromPid PyLong_FromLongLong
#define PyLong_AsPid PyLong_AsLongLong
#else
#error "sizeof(pid_t) is neither sizeof(int), sizeof(long) or sizeof(long long)"
#endif /* SIZEOF_PID_T */

#if SIZEOF_VOID_P == SIZEOF_INT
#  define _Py_PARSE_INTPTR "i"
#  define _Py_PARSE_UINTPTR "I"
#elif SIZEOF_VOID_P == SIZEOF_LONG
#  define _Py_PARSE_INTPTR "l"
#  define _Py_PARSE_UINTPTR "k"
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_VOID_P == SIZEOF_LONG_LONG
#  define _Py_PARSE_INTPTR "L"
#  define _Py_PARSE_UINTPTR "K"
#else
#  error "void* different in size from int, long and long long"
#endif /* SIZEOF_VOID_P */

/* Used by Python/mystrtoul.c, _PyBytes_FromHex(),
   _PyBytes_DecodeEscapeRecode(), etc. */
#ifndef Py_LIMITED_API
PyAPI_DATA(unsigned char) _PyLong_DigitValue[256];
#endif

/* _PyLong_Frexp returns a double x and an exponent e such that the
   true value is approximately equal to x * 2**e.  e is >= 0.  x is
   0.0 if and only if the input is 0 (in which case, e and x are both
   zeroes); otherwise, 0.5 <= abs(x) < 1.0.  On overflow, which is
   possible if the number of bits doesn't fit into a Py_ssize_t, sets
   OverflowError and returns -1.0 for x, 0 for e. 
   返回一个双精度浮点数x和一个指数e，使得真值约等于x * 2**e。e大于等于0. x为0.0
   当且仅当输入为0（在这种情况下，e与x都为0）；否则x的绝对值大于等于0.5且小于1.0。（0.5<=abs（x）<1.0）
   在溢出时（如果位数不符合Py_ssize_t的话这是可能发生的），设置为OverflowError并返回x为-1.0，e为0。
   */
#ifndef Py_LIMITED_API
PyAPI_FUNC(double) _PyLong_Frexp(PyLongObject *a, Py_ssize_t *e);
#endif

PyAPI_FUNC(double) PyLong_AsDouble(PyObject *);
PyAPI_FUNC(PyObject *) PyLong_FromVoidPtr(void *);
PyAPI_FUNC(void *) PyLong_AsVoidPtr(PyObject *);

PyAPI_FUNC(PyObject *) PyLong_FromLongLong(long long);
PyAPI_FUNC(PyObject *) PyLong_FromUnsignedLongLong(unsigned long long);
PyAPI_FUNC(long long) PyLong_AsLongLong(PyObject *);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLong(PyObject *);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLongMask(PyObject *);
PyAPI_FUNC(long long) PyLong_AsLongLongAndOverflow(PyObject *, int *);

PyAPI_FUNC(PyObject *) PyLong_FromString(const char *, char **, int);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) PyLong_FromUnicode(Py_UNICODE*, Py_ssize_t, int) Py_DEPRECATED(3.3);
PyAPI_FUNC(PyObject *) PyLong_FromUnicodeObject(PyObject *u, int base);
PyAPI_FUNC(PyObject *) _PyLong_FromBytes(const char *, Py_ssize_t, int);
#endif

#ifndef Py_LIMITED_API
/* _PyLong_Sign.  Return 0 if v is 0, -1 if v < 0, +1 if v > 0.
   v must not be NULL, and must be a normalized long.
   There are no error cases.
   _PyLong_Sign
   若v为0则返回0，v小于0则减1，大于0则加1。
   v不能为空，并且必须是一个标准化的长整型。
   没有错误案例。
*/
PyAPI_FUNC(int) _PyLong_Sign(PyObject *v);


/* _PyLong_NumBits.  Return the number of bits needed to represent the
   absolute value of a long.  For example, this returns 1 for 1 and -1, 2
   for 2 and -2, and 2 for 3 and -3.  It returns 0 for 0.
   v must not be NULL, and must be a normalized long.
   (size_t)-1 is returned and OverflowError set if the true result doesn't
   fit in a size_t.
   _PyLong_NumBits
   返回表示长字符串绝对值所需的位数。
   例如，1和-1会返回1，2和-2会返回2，3和-3会返回2。0会返回0。
   v不能为空，并且必须是一个标准化的长整型。
   如果真结果不符合size_t，返回(size_t)-1，设置为溢出错误。
*/
PyAPI_FUNC(size_t) _PyLong_NumBits(PyObject *v);

/* _PyLong_DivmodNear.  Given integers a and b, compute the nearest
   integer q to the exact quotient a / b, rounding to the nearest even integer
   in the case of a tie.  Return (q, r), where r = a - q*b.  The remainder r
   will satisfy abs(r) <= abs(b)/2, with equality possible only if q is
   even.

   _PyLong_DivmodNear
   给定整数a和b，计算最接近的整数q为精确商a/b，四舍五入到最近的偶数整数。
   返回（q，r），其中r=a-q*b。余数r将满足abs（r）<=abs（b）/2

*/
PyAPI_FUNC(PyObject *) _PyLong_DivmodNear(PyObject *, PyObject *);

/* _PyLong_FromByteArray:  View the n unsigned bytes as a binary integer in
   base 256, and return a Python int with the same numeric value.
   If n is 0, the integer is 0.  Else:
   If little_endian is 1/true, bytes[n-1] is the MSB and bytes[0] the LSB;
   else (little_endian is 0/false) bytes[0] is the MSB and bytes[n-1] the
   LSB.
   If is_signed is 0/false, view the bytes as a non-negative integer.
   If is_signed is 1/true, view the bytes as a 2's-complement integer,
   non-negative if bit 0x80 of the MSB is clear, negative if set.
   Error returns:
   + Return NULL with the appropriate exception set if there's not
     enough memory to create the Python int.

	_PyLong_FromByteArray
	 将n个无符号字节作为二进制整数查看以256为基数，并返回具有相同数值的int类型的值。
	 如果n为0，则整数为0
	 如果little_endian为1/true，则字节[n-1]为MSB，字节[0]为LSB；
	 else（little_endian为0/false）字节[0]为MSB，字节[n-1]为LSB。
	 如果is_signed为0/false，则将字节视为非负整数。
	 如果is_signed为1/true，则将字节视为2的补码整数。
	 如果MSB的位0x80为空，则为非负；如果已设置，则为负。
	 如果没有异常，则返回NULL并设置相应的异常
*/
PyAPI_FUNC(PyObject *) _PyLong_FromByteArray(
    const unsigned char* bytes, size_t n,
    int little_endian, int is_signed);

/* _PyLong_AsByteArray: Convert the least-significant 8*n bits of long
   v to a base-256 integer, stored in array bytes.  Normally return 0,
   return -1 on error.
   If little_endian is 1/true, store the MSB at bytes[n-1] and the LSB at
   bytes[0]; else (little_endian is 0/false) store the MSB at bytes[0] and
   the LSB at bytes[n-1].
   If is_signed is 0/false, it's an error if v < 0; else (v >= 0) n bytes
   are filled and there's nothing special about bit 0x80 of the MSB.
   If is_signed is 1/true, bytes is filled with the 2's-complement
   representation of v's value.  Bit 0x80 of the MSB is the sign bit.
   Error returns (-1):
   + is_signed is 0 and v < 0.  TypeError is set in this case, and bytes
     isn't altered.
   + n isn't big enough to hold the full mathematical value of v.  For
     example, if is_signed is 0 and there are more digits in the v than
     fit in n; or if is_signed is 1, v < 0, and n is just 1 bit shy of
     being large enough to hold a sign bit.  OverflowError is set in this
     case, but bytes holds the least-significant n bytes of the true value.

	_PyLong_AsByteArray
	 转换长序列的最低有效8*n位v到一个基256整数，存储在数组字节中。通常返回0，错误时返回-1。
	 如果little_endian为1/true，则将MSB存储在字节[n-1]处，将LSB存储在字节[0]；else（little_endian为0/false）将MSB存储在字节[0]处，并字节[n-1]处的LSB。
	 如果is_signed为0/false，则v<0为错误；else（v>=0）n字节已填充，并且MSB的位0x80没有任何特殊之处。
	 若is_signed为1/true，则字节用2的补码填充v值的表示。MSB的位0x80是符号位。
	 错误返回（-1）：
	 +符号为0且v<0。在这种情况下设置了TypeError，字节没有改变。
	 +n不足以容纳v的全部数学值。例如，if_signed为0且v中的数字多于适合n；或者如果符号为1，v<0，n只差一点大到足以容纳一个符号位。
	 此字段中设置了OverflowerError大小写为，但字节保留真值的最低有效n字节。

*/
PyAPI_FUNC(int) _PyLong_AsByteArray(PyLongObject* v,
    unsigned char* bytes, size_t n,
    int little_endian, int is_signed);

/* _PyLong_FromNbInt: Convert the given object to a PyLongObject
   using the nb_int slot, if available.  Raise TypeError if either the
   nb_int slot is not available or the result of the call to nb_int
   returns something not of type int.

   _PyLong_FromNbInt
   将给定对象转换为PyLongObject使用nb_int插槽。
   如果nb_int插槽不可用或调用nb_int的结果返回非int类型的内容。
*/
PyAPI_FUNC(PyLongObject *)_PyLong_FromNbInt(PyObject *);

/* _PyLong_Format: Convert the long to a string object with given base,
   appending a base prefix of 0[box] if base is 2, 8 or 16. 
   
   _PyLong_Format
   将长字符串对象转换为具有给定基的字符串对象，如果基数为2、8或16，则追加基数前缀0[box]。
*/
PyAPI_FUNC(PyObject *) _PyLong_Format(PyObject *obj, int base);

PyAPI_FUNC(int) _PyLong_FormatWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    int base,
    int alternate);

PyAPI_FUNC(char*) _PyLong_FormatBytesWriter(
    _PyBytesWriter *writer,
    char *str,
    PyObject *obj,
    int base,
    int alternate);

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). 
   根据PEP 3101中定义的格式规范格式化对象
   */
PyAPI_FUNC(int) _PyLong_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);
#endif /* Py_LIMITED_API */

/* These aren't really part of the int object, but they're handy. The
   functions are in Python/mystrtoul.c.
   这些实际上不是int对象的一部分，但它们很方便。
   这个函数的格式为Python/mystrtoul.c
 */
PyAPI_FUNC(unsigned long) PyOS_strtoul(const char *, char **, int);
PyAPI_FUNC(long) PyOS_strtol(const char *, char **, int);

#ifndef Py_LIMITED_API
/* 
For use by the gcd function in mathmodule.c 
供mathmodule.c中的gcd函数使用
*/
PyAPI_FUNC(PyObject *) _PyLong_GCD(PyObject *, PyObject *);
#endif /* !Py_LIMITED_API */

#ifndef Py_LIMITED_API
PyAPI_DATA(PyObject *) _PyLong_Zero;
PyAPI_DATA(PyObject *) _PyLong_One;
#endif

#ifdef Py_REF_DEBUG
/* trace longobject's create operation*/
//PyAPI_DATA(Py_ssize_t) my_long_create_step;
//PyAPI_FUNC(Py_ssize_t) _Py_GetMy_Long_Create_Step(void);
//#define _Py_MY_LONG_CREATE_TRACE_ON     my_long_create_step = 0
//#define _Py_MY_LONG_CREATE_TRACE_OFF    my_long_create_step = -1
//#define _Py_MY_LONG_CREATE_TRACE_NEXT    my_long_create_step++
/* if my_long_create_step > -1, then trace mode on*/
//#define Py_MYLONG_CREATE_TRACE_OFF -1
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGOBJECT_H */
