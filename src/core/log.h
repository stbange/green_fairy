#ifndef GF_CORE_LOG_H
#define GF_CORE_LOG_H

typedef enum GFLogLevel {
    GF_LOG_DEBUG = 0,
    GF_LOG_INFO  = 1,
    GF_LOG_WARN  = 2,
    GF_LOG_ERROR = 3
} GFLogLevel;

typedef void (*GFLogSinkFn)(GFLogLevel level, const char *tag, const char *message, void *user_data);

void gf_log_set_level(GFLogLevel min_level);
GFLogLevel gf_log_get_level(void);
void gf_log_set_sink(GFLogSinkFn sink, void *user_data);
void gf_log_write(GFLogLevel level, const char *tag, const char *fmt, ...);

#define GF_LOGD(tag, ...) gf_log_write(GF_LOG_DEBUG, tag, __VA_ARGS__)
#define GF_LOGI(tag, ...) gf_log_write(GF_LOG_INFO,  tag, __VA_ARGS__)
#define GF_LOGW(tag, ...) gf_log_write(GF_LOG_WARN,  tag, __VA_ARGS__)
#define GF_LOGE(tag, ...) gf_log_write(GF_LOG_ERROR, tag, __VA_ARGS__)

#endif
