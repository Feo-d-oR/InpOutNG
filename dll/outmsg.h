#pragma once

#include "datadll.h"

EXTERN_C_START

/* msg() flags */

#define M_DEBUG_LEVEL     (0x0F)         /* debug level mask */

#define M_FATAL           (1<<4)         /* exit program */
#define M_NONFATAL        (1<<5)         /* non-fatal error */
#define M_WARN            (1<<6)         /* call syslog with LOG_WARNING */
#define M_DEBUG           (1<<7)

#define M_ERRNO           (1<<8)         /* show errno description */

#define M_NOMUTE          (1<<11)        /* don't do mute processing */
#define M_NOPREFIX        (1<<12)        /* don't show date/time prefix */
#define M_USAGE_SMALL     (1<<13)        /* fatal options error, call usage_small */
#define M_MSG_VIRT_OUT    (1<<14)        /* output message through msg_status_output callback */
#define M_OPTERR          (1<<15)        /* print "Options error:" prefix */
#define M_NOLF            (1<<16)        /* don't print new line */
#define M_NOIPREFIX       (1<<17)        /* don't print instance prefix */

/* flag combinations which are frequently used */
#define M_ERR     (M_FATAL | M_ERRNO)
#define M_USAGE   (M_USAGE_SMALL | M_NOPREFIX | M_OPTERR)
#define M_CLIENT  (M_MSG_VIRT_OUT | M_NOMUTE | M_NOIPREFIX)

/* Macro to ensure (and teach static analysis tools) we exit on fatal errors */
#ifdef _MSC_VER
#pragma warning(disable: 4127) /* EXIT_FATAL(flags) macro raises "warning C4127: conditional expression is constant" on each non M_FATAL invocation. */
#endif
#define EXIT_FATAL(flags) do { if ((flags) & M_FATAL) {_exit(1);}} while (FALSE)

#define HAVE_VARARG_MACROS

#ifdef ENABLE_MSG
#define msg(flags, ...) outmsg((flags), __VA_ARGS__)
#else
#define msg(flags, ...)
#endif

#ifdef ENABLE_DEBUG
#define dmsg(flags, ...) do { if (msg_test(flags)) {outmsg((flags), __VA_ARGS__);} EXIT_FATAL(flags); } while (FALSE)
#else
#define dmsg(flags, ...)
#endif

void outmsg(const unsigned int flags, const TCHAR* format, ...);     /* should be called via msg above */

void outmsg_va(const unsigned int flags, const TCHAR* format, va_list arglist);

EXTERN_C_END
