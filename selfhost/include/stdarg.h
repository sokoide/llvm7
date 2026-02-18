#ifndef SELFHOST_STDARG_H
#define SELFHOST_STDARG_H

// For clang (normal build), use builtin va_list
#ifdef __clang__
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#else
// For self-hosted compiler, use pointer-based va_list
typedef void* va_list;
#define va_start(ap, last) ((ap) = (va_list)(&last + 1))
#define va_arg(ap, type) (*(type*)(ap)++)
#define va_end(ap) ((ap) = (va_list)0)
#endif

#endif
