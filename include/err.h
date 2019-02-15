#ifndef ERR_H
#define ERR_H

#ifdef __cplusplus
extern "C" {
#endif

#define ERR_EXIT(format, ...) err_exit("%s:%d | " format, __FILE__, __LINE__, __VA_ARGS__)
#define ERR_SYS(format, ...) err_sys("%s:%d | " format, __FILE__, __LINE__, __VA_ARGS__)

    /// Simple error handler
    _Noreturn void
    err_exit(const char *format, ...);

    /// Errno error handler
    _Noreturn void
    err_sys(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ERR_H */