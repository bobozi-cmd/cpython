#ifndef Py_LONGOBJECT_H
#define Py_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* 
Long (arbitrary precision) integer object interface 
����������ӿڣ����⾫�ȣ�
*/

typedef struct _longobject PyLongObject; /* Revealed in longintrepr.h ��longintrepr.h����ʾ  */

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
   
   �ڽ��������������á�����PyInt->PyLongת��������������Ա����������Ϣ��

   */
#define PyLong_AS_LONG(op) PyLong_AsLong(op)

/* Issue #1983: pid_t can be longer than a C long on some systems 
   ���� #1983����ĳЩϵͳ��pid_t�ĳ��ȿ��ܳ���C��long
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
   ����һ��˫���ȸ�����x��һ��ָ��e��ʹ����ֵԼ����x * 2**e��e���ڵ���0. xΪ0.0
   ���ҽ�������Ϊ0������������£�e��x��Ϊ0��������x�ľ���ֵ���ڵ���0.5��С��1.0����0.5<=abs��x��<1.0��
   �����ʱ�����λ��������Py_ssize_t�Ļ����ǿ��ܷ����ģ�������ΪOverflowError������xΪ-1.0��eΪ0��
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
   ��vΪ0�򷵻�0��vС��0���1������0���1��
   v����Ϊ�գ����ұ�����һ����׼���ĳ����͡�
   û�д�������
*/
PyAPI_FUNC(int) _PyLong_Sign(PyObject *v);


/* _PyLong_NumBits.  Return the number of bits needed to represent the
   absolute value of a long.  For example, this returns 1 for 1 and -1, 2
   for 2 and -2, and 2 for 3 and -3.  It returns 0 for 0.
   v must not be NULL, and must be a normalized long.
   (size_t)-1 is returned and OverflowError set if the true result doesn't
   fit in a size_t.
   _PyLong_NumBits
   ���ر�ʾ���ַ�������ֵ�����λ����
   ���磬1��-1�᷵��1��2��-2�᷵��2��3��-3�᷵��2��0�᷵��0��
   v����Ϊ�գ����ұ�����һ����׼���ĳ����͡�
   �������������size_t������(size_t)-1������Ϊ�������
*/
PyAPI_FUNC(size_t) _PyLong_NumBits(PyObject *v);

/* _PyLong_DivmodNear.  Given integers a and b, compute the nearest
   integer q to the exact quotient a / b, rounding to the nearest even integer
   in the case of a tie.  Return (q, r), where r = a - q*b.  The remainder r
   will satisfy abs(r) <= abs(b)/2, with equality possible only if q is
   even.

   _PyLong_DivmodNear
   ��������a��b��������ӽ�������qΪ��ȷ��a/b���������뵽�����ż��������
   ���أ�q��r��������r=a-q*b������r������abs��r��<=abs��b��/2

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
	 ��n���޷����ֽ���Ϊ�����������鿴��256Ϊ�����������ؾ�����ͬ��ֵ��int���͵�ֵ��
	 ���nΪ0��������Ϊ0
	 ���little_endianΪ1/true�����ֽ�[n-1]ΪMSB���ֽ�[0]ΪLSB��
	 else��little_endianΪ0/false���ֽ�[0]ΪMSB���ֽ�[n-1]ΪLSB��
	 ���is_signedΪ0/false�����ֽ���Ϊ�Ǹ�������
	 ���is_signedΪ1/true�����ֽ���Ϊ2�Ĳ���������
	 ���MSB��λ0x80Ϊ�գ���Ϊ�Ǹ�����������ã���Ϊ����
	 ���û���쳣���򷵻�NULL��������Ӧ���쳣
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
	 ת�������е������Ч8*nλv��һ����256�������洢�������ֽ��С�ͨ������0������ʱ����-1��
	 ���little_endianΪ1/true����MSB�洢���ֽ�[n-1]������LSB�洢���ֽ�[0]��else��little_endianΪ0/false����MSB�洢���ֽ�[0]�������ֽ�[n-1]����LSB��
	 ���is_signedΪ0/false����v<0Ϊ����else��v>=0��n�ֽ�����䣬����MSB��λ0x80û���κ�����֮����
	 ��is_signedΪ1/true�����ֽ���2�Ĳ������vֵ�ı�ʾ��MSB��λ0x80�Ƿ���λ��
	 ���󷵻أ�-1����
	 +����Ϊ0��v<0�������������������TypeError���ֽ�û�иı䡣
	 +n����������v��ȫ����ѧֵ�����磬if_signedΪ0��v�е����ֶ����ʺ�n�������������Ϊ1��v<0��nֻ��һ�����������һ������λ��
	 ���ֶ���������OverflowerError��СдΪ�����ֽڱ�����ֵ�������Чn�ֽڡ�

*/
PyAPI_FUNC(int) _PyLong_AsByteArray(PyLongObject* v,
    unsigned char* bytes, size_t n,
    int little_endian, int is_signed);

/* _PyLong_FromNbInt: Convert the given object to a PyLongObject
   using the nb_int slot, if available.  Raise TypeError if either the
   nb_int slot is not available or the result of the call to nb_int
   returns something not of type int.

   _PyLong_FromNbInt
   ����������ת��ΪPyLongObjectʹ��nb_int��ۡ�
   ���nb_int��۲����û����nb_int�Ľ�����ط�int���͵����ݡ�
*/
PyAPI_FUNC(PyLongObject *)_PyLong_FromNbInt(PyObject *);

/* _PyLong_Format: Convert the long to a string object with given base,
   appending a base prefix of 0[box] if base is 2, 8 or 16. 
   
   _PyLong_Format
   �����ַ�������ת��Ϊ���и��������ַ��������������Ϊ2��8��16����׷�ӻ���ǰ׺0[box]��
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
   ����PEP 3101�ж���ĸ�ʽ�淶��ʽ������
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
   ��Щʵ���ϲ���int�����һ���֣������Ǻܷ��㡣
   ��������ĸ�ʽΪPython/mystrtoul.c
 */
PyAPI_FUNC(unsigned long) PyOS_strtoul(const char *, char **, int);
PyAPI_FUNC(long) PyOS_strtol(const char *, char **, int);

#ifndef Py_LIMITED_API
/* 
For use by the gcd function in mathmodule.c 
��mathmodule.c�е�gcd����ʹ��
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
