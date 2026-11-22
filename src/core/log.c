#include "core/log.h"

#include <stdarg.h>
#include <stdio.h>

static GFLogLevel g_min_level = GF_LOG_INFO;
static GFLogSinkFn g_sink = NULL;
static void *g_sink_user = NULL;

static const char *level_name(GFLogLevel level) {
    switch (level) {
        case GF_LOG_DEBUG: return "DEBUG";
        case GF_LOG_INFO:  return "INFO";
        case GF_LOG_WARN:  return "WARN";
        case GF_LOG_ERROR: return "ERROR";
    }
    return "INFO";
}

void gf_log_set_level(GFLogLevel min_level) {
    g_min_level = min_level;
}

GFLogLevel gf_log_get_level(void) {
    return g_min_level;
}

void gf_log_set_sink(GFLogSinkFn sink, void *user_data) {
    g_sink = sink;
    g_sink_user = user_data;
}

void gf_log_write(GFLogLevel level, const char *tag, const char *fmt, ...) {
    if (level < g_min_level || !fmt) {
        return;
    }
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    const char *safe_tag = tag ? tag : "gf";
    if (g_sink) {
        g_sink(level, safe_tag, buf, g_sink_user);
    } else {
        FILE *out = (level >= GF_LOG_WARN) ? stderr : stdout;
        fprintf(out, "[%s][%s] %s\n", level_name(level), safe_tag, buf);
    }
}
