#include "log.h"

void 
log_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsyslog(LOG_ERR, format, args);
	va_end(args);
}

void 
log_info(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsyslog(LOG_INFO, format, args);
	va_end(args);
}

void
log_start(int options, int facility) {
	openlog(LOG_IDENT, options, facility);
	syslog(LOG_INFO, "start (build %s %s)", COMMIT, DATE);

}

void
log_stop(void) {
	syslog(LOG_INFO, "finish");
	closelog();
}
