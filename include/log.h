#ifndef LOG_H
#define LOG_H

#include <syslog.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_IDENT       "vlt"
#define LOG_OPTS        LOG_PID | LOG_CONS | LOG_NOWAIT | LOG_PERROR
#define LOG_FACILITY    LOG_LOCAL0

#define INFO(format, ...) log_info("%s:%d | " format, __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(format, ...) log_error("%s:%d | " format, __FILE__, __LINE__, __VA_ARGS__)

    void
    log_error(const char *format, ...);

    void
    log_info(const char *format, ...);

    void
    log_start(int options, int facility);

    void
    log_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
