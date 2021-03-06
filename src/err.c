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

_Noreturn void 
err_exit(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsyslog(LOG_ERR, format, args);
	va_end(args);
	log_stop();
	exit(EXIT_FAILURE);
}

_Noreturn void 
err_sys(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsyslog(LOG_ERR, format, args);
	syslog(LOG_ERR, "[Errno %d] %s", errno, strerror(errno));
	va_end(args);
	log_stop();
	exit(EXIT_FAILURE);
}
