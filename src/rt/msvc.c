/**
* This module provides MS VC runtime helper function that
* wrap differences between different versions of the MS C runtime
*
* Copyright: Copyright Digital Mars 2015.
* License: Distributed under the
*      $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost Software License 1.0).
*    (See accompanying file LICENSE)
* Source:    $(DRUNTIMESRC rt/_msvc.c)
* Authors:   Rainer Schuetze
*/

#if defined _M_IX86
    #define C_PREFIX "_"
#elif defined _M_X64 || defined _M_ARM || defined _M_ARM64
    #define C_PREFIX ""
#else
    #error Unsupported architecture
#endif

#define DECLARE_ALTERNATE_NAME(name, alternate_name)  \
    __pragma(comment(linker, "/alternatename:" C_PREFIX #name "=" C_PREFIX #alternate_name))



/*** stdio ***/

struct _iobuf
{
    char* _ptr;
    int   _cnt;  // _cnt and _base exchanged for VS2015
    char* _base;
    int   _flag;
    int   _file;
    int   _charbuf;
    int   _bufsiz;
    char* _tmpfname;
    // additional members in VS2015
};

typedef struct _iobuf FILE;
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

FILE* __acrt_iob_func(int hnd);     // VS2015+
FILE* __iob_func();                 // VS2013-

int _set_output_format(int format); // VS2013-

//extern const char* __acrt_iob_func;
extern const char* _nullfunc = 0;

DECLARE_ALTERNATE_NAME (__acrt_iob_func, _nullfunc);
DECLARE_ALTERNATE_NAME (__iob_func, _nullfunc);
DECLARE_ALTERNATE_NAME (_set_output_format, _nullfunc);

void init_msvc()
{
    if (&__acrt_iob_func != (void*) &_nullfunc)
    {
        stdin = __acrt_iob_func(0);
        stdout = __acrt_iob_func(1);
        stderr = __acrt_iob_func(2);
    }
    else if (&__iob_func != (void*) &_nullfunc)
    {
        FILE* fp = __iob_func();
        stdin = fp;
        stdout = fp + 1;
        stderr = fp + 2;
    }
    if (&_set_output_format != (void*) &_nullfunc)
    {
        const int _TWO_DIGIT_EXPONENT = 1;
        _set_output_format(_TWO_DIGIT_EXPONENT);
    }
}

// VS2015+ provides C99-conformant (v)snprintf functions, so weakly
// link to legacy _(v)snprintf (not C99-conformant!) for VS2013- only

DECLARE_ALTERNATE_NAME (snprintf, _snprintf);
DECLARE_ALTERNATE_NAME (vsnprintf, _vsnprintf);

// VS2013- implements these functions as macros, VS2015+ provides symbols

DECLARE_ALTERNATE_NAME (_fputc_nolock, _msvc_fputc_nolock);
DECLARE_ALTERNATE_NAME (_fgetc_nolock, _msvc_fgetc_nolock);
DECLARE_ALTERNATE_NAME (rewind, _msvc_rewind);
DECLARE_ALTERNATE_NAME (clearerr, _msvc_clearerr);
DECLARE_ALTERNATE_NAME (feof, _msvc_feof);
DECLARE_ALTERNATE_NAME (ferror, _msvc_ferror);
DECLARE_ALTERNATE_NAME (fileno, _msvc_fileno);

// VS2013- helper functions
int _filbuf(FILE* fp);
int _flsbuf(int c, FILE* fp);

DECLARE_ALTERNATE_NAME(_filbuf, _nullfunc);
DECLARE_ALTERNATE_NAME(_flsbuf, _nullfunc);

int _msvc_fputc_nolock(int c, FILE* fp)
{
    fp->_cnt = fp->_cnt - 1;
    if (fp->_cnt >= 0)
    {
        *(fp->_ptr) = (char)c;
        fp->_ptr = fp->_ptr + 1;
        return (char)c;
    }
    else
        return _flsbuf(c, fp);
}

int _msvc_fgetc_nolock(FILE* fp)
{
    fp->_cnt = fp->_cnt - 1;
    if (fp->_cnt >= 0)
    {
        char c = *(fp->_ptr);
        fp->_ptr = fp->_ptr + 1;
        return c;
    }
    else
        return _filbuf(fp);
}

enum
{
    SEEK_SET = 0,
    _IOEOF   = 0x10,
    _IOERR   = 0x20
};

int fseek(FILE* fp, long off, int whence);

void _msvc_rewind(FILE* stream)
{
    fseek(stream, 0L, SEEK_SET);
    stream->_flag = stream->_flag & ~_IOERR;
}

void _msvc_clearerr(FILE* stream)
{
    stream->_flag = stream->_flag & ~(_IOERR | _IOEOF);
}

int  _msvc_feof(FILE* stream)
{
    return stream->_flag & _IOEOF;
}

int  _msvc_ferror(FILE* stream)
{
    return stream->_flag & _IOERR;
}

int  _msvc_fileno(FILE* stream)
{
    return stream->_file;
}



/*** math ***/

// The x86 runtime lacks most 32-bit float *f math functions.
// Provide alternate implementations using the 64-bit double version.
#if defined _M_IX86

#define IMPL(baseName) \
    __pragma(comment(linker, "/alternatename:" C_PREFIX #baseName "f=" C_PREFIX "_msvc_" #baseName "f")); \
    double baseName(double x); \
    float _msvc_ ## baseName ## f(float x) { return (float)baseName(x); }
#define IMPL2(baseName) \
    __pragma(comment(linker, "/alternatename:" C_PREFIX #baseName "f=" C_PREFIX "_msvc_" #baseName "f")); \
    double baseName(double x, double y); \
    float _msvc_ ## baseName ## f(float x, float y) { return (float)baseName(x, y); }

IMPL(acos);
IMPL(asin);
IMPL(atan);
IMPL2(atan2);
IMPL(cos);
IMPL(sin);
IMPL(tan);
IMPL(cosh);
IMPL(sinh);
IMPL(tanh);
IMPL(exp);
IMPL(log);
IMPL(log10);

DECLARE_ALTERNATE_NAME (modff,  _msvc_modff);
double modf(double value, double *iptr);
float _msvc_modff(float value, float *iptr)
{
    double di;
    float result = (float)modf(value, &di);
    *iptr = (float)di;
    return result;
}

IMPL2(pow);
IMPL(sqrt);
IMPL(ceil);
IMPL(floor);
IMPL2(fmod);

#undef IMPL
#undef IMPL2

#endif // _M_IX86
