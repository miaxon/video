/**
 * \file err.c
 *  Contains error message functions and simple error handlers
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>

#include "err.h"
#include "log.h"
#define BUFSIZE 1024

_Noreturn void
err_exit (const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buf[BUFSIZE] = {0};
	snprintf(buf, BUFSIZE - 1, "[EXIT] %s\n", format);
	vprintf(buf, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

_Noreturn void
err_sys (const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buf[BUFSIZE] = {0};
	snprintf(buf, BUFSIZE - 1, "[SYS Errno %d] %s\n", errno, format);
	vprintf(buf, args);
	va_end(args);
	exit(EXIT_FAILURE);
}
