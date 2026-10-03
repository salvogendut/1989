 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Standard write_log that writes to the console
  *
  * Copyright 2001 Bernd Schmidt
  */
#include "sysconfig.h"
#include "sysdeps.h"

#include "log.h"

/* Previous: the CPU core emits a lot of informational output (memory map,
 * CPU build, MMU fixups, ...). Gate it behind the 1989 "Debugging" toggle so
 * the terminal stays quiet by default. */
void write_log (const char *fmt, ...)
{
	va_list ap;

	if (!Log_DebugEnabled())
		return;

	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);
}

void f_out (void *f, const TCHAR *format, ...)
{
	va_list parms;

	if (f == NULL)
		return;

	va_start (parms, format);
	vfprintf (f, format, parms);
	va_end (parms);
}
