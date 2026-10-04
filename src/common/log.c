#define _GNU_SOURCE
#include "mango/common/log.h"
#include "mango/common/util.h"
#include "mango/config/config_error_store.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void mango_log_init(enum wlr_log_importance verbosity,
					wlr_log_func_t callback) {
	wlr_log_init(verbosity, callback);
}

void mango_error_impl(bool log, enum wlr_log_importance verbosity,
					  const char *file, int line, const char *fmt, ...) {
	va_list args;
	va_list args_copy;
	bool capture = false;

	va_start(args, fmt);
	if (!log) {
		capture = config_error_store_active();
		if (capture)
			va_copy(args_copy, args);
		if (verbosity == WLR_ERROR) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[0m ");
			vfprintf(stderr, fmt, args);
		} else if (verbosity == WLR_INFO) {
			vfprintf(stderr, fmt, args);
		} else if (verbosity == WLR_DEBUG) {
			fprintf(stderr, "\033[1;32m[DEBUG]:\033[0m ");
			vfprintf(stderr, fmt, args);
		}
	} else {
		char *prefixed = string_printf("[%s:%d] %s", file, line, fmt);
		if (prefixed) {
			_wlr_vlog(verbosity, prefixed, args);
			free(prefixed);
		} else {
			_wlr_vlog(verbosity, fmt, args);
		}
	}

	if (capture) {
		char buf[4096];
		if (vsnprintf(buf, sizeof(buf), fmt, args_copy) >= 0)
			config_error_store_record(buf);
		va_end(args_copy);
	}

	va_end(args);
}
