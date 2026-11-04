#ifndef REDEG_LOGGER_H
#define REDEG_LOGGER_H

enum {
    LT_DEBUG = 0,
    LT_INFO,
    LT_WARN,
    LT_ERROR,
    LT_FATAL,
    LT_SUCCESS,
    LT_WAIT
};

void _log_print(int level, const char *fmt, ...);
const char *log_get_path(void);

#define l_debug(...) _log_print(LT_DEBUG, __VA_ARGS__)
#define l_info(...)  _log_print(LT_INFO, __VA_ARGS__)
#define l_warn(...)  _log_print(LT_WARN, __VA_ARGS__)
#define l_error(...) _log_print(LT_ERROR, __VA_ARGS__)
#define l_fatal(...) _log_print(LT_FATAL, __VA_ARGS__)
#define l_success(...) _log_print(LT_SUCCESS, __VA_ARGS__)

#endif
