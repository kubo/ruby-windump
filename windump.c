#include <ruby.h>
#include <signal.h>
#include "minidump.h"

#ifdef HAVE_RUBY_ENCODING_H
#include <ruby/encoding.h>
#endif

#ifndef _pxcptinfoptrs
extern void ** __cdecl __pxcptinfoptrs(void);
#define _pxcptinfoptrs  (*__pxcptinfoptrs())
#endif

#define DEFAULT_DUMP_FILENAME "minidump.dmp"

static LPTOP_LEVEL_EXCEPTION_FILTER orig_top_level_exception_filter;
static void (__cdecl *orig_sigsegv_handler)(int);
static char *dump_filename = NULL;
static MINIDUMP_TYPE minidump_type = MiniDumpNormal;

/*
 * Mingw32 ruby calls top_level_exception_filter on segmentation fault.
 * But mswin32 ruby doesn't.
 */
static LONG CALLBACK top_level_exception_filter(PEXCEPTION_POINTERS excptr)
{
    switch (excptr->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        fprintf(stderr, "Caught EXCEPTION_ACCESS_VIOLATION\n");
        minidump_dump(dump_filename ? dump_filename : DEFAULT_DUMP_FILENAME, minidump_type, excptr);
    }
    return orig_top_level_exception_filter(excptr);
}

/*
 * Though both mingw32 and mswin32 ruby call sigsegv_handler on
 * segmentation fault, _pxcptinfoptrs is null in mingw32.
 * Thus mswin32 ruby only geneates a minidump file by this handler.
 */
static void sigsegv_handler(int signo)
{
    PEXCEPTION_POINTERS excptr = _pxcptinfoptrs;
    if (excptr != NULL) {
        fprintf(stderr, "Caught SIGSEGV\n");
        minidump_dump(dump_filename ? dump_filename : DEFAULT_DUMP_FILENAME, minidump_type, excptr);
    }
    orig_sigsegv_handler(signo);
}

static VALUE windump_set_dump_file(VALUE klass, VALUE file)
{
#ifdef FilePathValue
    FilePathValue(file);
#else
    SafeStringValue(file);
#endif
#ifdef HAVE_RUBY_ENCODING_H
    file = rb_str_export_to_enc(file, rb_filesystem_encoding());
#endif
    if (dump_filename != NULL) {
        free(dump_filename);
        dump_filename = NULL;
    }
    dump_filename = strdup(StringValueCStr(file));
    return file;
}

static VALUE windump_set_raw_dump_type(VALUE klass, VALUE type)
{
    minidump_type = NUM2INT(type);
    return type;
}

static VALUE windump_get_raw_dump_type(VALUE klass)
{
    return INT2NUM(minidump_type);
}

static HMODULE hDLL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        hDLL = hinstDLL;
    }
    return TRUE;
}

void Init_windump(void)
{
    VALUE mWindump;
    const char *errmsg;

    errmsg = minidump_init(hDLL);
    if (errmsg != NULL) {
        rb_raise(rb_eRuntimeError, "%s", errmsg);
    }
    orig_top_level_exception_filter = SetUnhandledExceptionFilter(top_level_exception_filter);
    orig_sigsegv_handler = signal(SIGSEGV, sigsegv_handler);

    mWindump = rb_define_module("Windump");
    rb_define_singleton_method(mWindump, "dump_file=", windump_set_dump_file, 1);
    rb_define_singleton_method(mWindump, "raw_dump_type=", windump_set_raw_dump_type, 1);
    rb_define_singleton_method(mWindump, "raw_dump_type", windump_get_raw_dump_type, 0);
}
