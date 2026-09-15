#include "mango/manage/xwayland_primary.h"

#include "mango/common/server.h"
#include "mango/manage/monitor.h"

#ifdef XWAYLAND
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/xwayland.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>

#define XWL_NAME_MAX 64
#define XWL_CACHE_MAX 16

static xcb_connection_t *conn = NULL;
static struct wl_event_source *conn_source = NULL;
static xcb_window_t root = XCB_NONE;
static char display_name[XWL_NAME_MAX];
static char target_name[XWL_NAME_MAX];

static char cache_name[XWL_CACHE_MAX][XWL_NAME_MAX];
static xcb_randr_output_t cache_output[XWL_CACHE_MAX];
static int32_t cache_len = 0;
static bool cache_valid = false;

static bool resources_pending = false;
static xcb_randr_get_screen_resources_cookie_t resources_cookie;
static bool info_pending = false;
static xcb_randr_get_output_info_cookie_t info_cookie;
static xcb_randr_output_t *outputs = NULL;
static int32_t outputs_len = 0, outputs_idx = 0;
static struct wl_event_source *retry_timer = NULL;
static int32_t retry_count = 0;

/* A dead XWayland accepts connections but never answers, so probe it first.
 * xcb_connect() does the handshake on this thread and would block forever. */
static bool xwayland_display_alive(const char *display) {
	if (display[0] != ':') {
		return false;
	}
	char path[64];
	snprintf(path, sizeof(path), "/tmp/.X11-unix/X%d", atoi(display + 1));

	int32_t fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		return false;
	}

	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 &&
		errno != EINPROGRESS) {
		close(fd);
		return false;
	}

	bool alive = false;
	struct pollfd pfd = {.fd = fd, .events = POLLOUT};
	if (poll(&pfd, 1, 300) > 0) {
		/* X setup request (protocol 11, no auth); any reply means it lives. */
		static const char setup[12] = {'l', 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		if (write(fd, setup, sizeof(setup)) == (ssize_t)sizeof(setup)) {
			char reply[8];
			pfd.events = POLLIN;
			if (poll(&pfd, 1, 300) > 0 &&
				read(fd, reply, sizeof(reply)) == (ssize_t)sizeof(reply)) {
				alive = true;
			}
		}
	}

	close(fd);
	return alive;
}

static void xwayland_primary_close(void) {
	if (conn_source) {
		wl_event_source_remove(conn_source);
		conn_source = NULL;
	}
	if (conn) {
		xcb_disconnect(conn);
		conn = NULL;
	}
	root = XCB_NONE;
	resources_pending = false;
	info_pending = false;
	free(outputs);
	outputs = NULL;
	outputs_len = outputs_idx = 0;
	cache_len = 0;
	cache_valid = false;
}

static void xwayland_primary_apply(void);

static int32_t xwayland_primary_retry(void *data) {
	/* XWayland may have announced its output list by now. */
	cache_valid = false;
	cache_len = 0;
	xwayland_primary_apply();
	return 0;
}

static bool xwayland_primary_connect(void) {
	if (conn) {
		return true;
	}
	if (!xwayland_display_alive(display_name)) {
		return false;
	}

	int32_t screen_num = 0;
	conn = xcb_connect(display_name, &screen_num);
	if (!conn || xcb_connection_has_error(conn)) {
		xwayland_primary_close();
		return false;
	}

	xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(conn));
	for (int32_t i = 0; i < screen_num && it.rem; i++) {
		xcb_screen_next(&it);
	}
	if (!it.rem) {
		xwayland_primary_close();
		return false;
	}
	root = it.data->root;
	cache_valid = false;
	retry_count = 0;
	return true;
}

static void xwayland_primary_build_cache(void) {
	free(outputs);
	outputs = NULL;
	outputs_len = outputs_idx = 0;
	cache_len = 0;
	cache_valid = false;
	resources_pending = true;
	resources_cookie = xcb_randr_get_screen_resources(conn, root);
	xcb_flush(conn);
}

static void xwayland_primary_apply(void) {
	if (!target_name[0]) {
		return;
	}
	if (!xwayland_primary_connect()) {
		if (retry_timer && retry_count++ < 5) {
			wl_event_source_timer_update(retry_timer, 200);
		}
		return;
	}
	if (!cache_valid) {
		xwayland_primary_build_cache();
		return;
	}
	for (int32_t i = 0; i < cache_len; i++) {
		if (strncmp(cache_name[i], target_name, XWL_NAME_MAX) != 0) {
			continue;
		}
		xcb_randr_set_output_primary(conn, root, cache_output[i]);
		xcb_flush(conn);
		retry_count = 0;
		return;
	}

	/* The output list may have changed; rebuild it a few times. */
	if (retry_count++ < 3) {
		cache_valid = false;
		xwayland_primary_build_cache();
	}
}

static int32_t xwayland_primary_ready(int32_t fd, uint32_t mask, void *data) {
	if (!conn || (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR))) {
		xwayland_primary_close();
		return 0;
	}

	if (resources_pending) {
		xcb_randr_get_screen_resources_reply_t *resources =
			xcb_randr_get_screen_resources_reply(conn, resources_cookie, NULL);
		if (resources) {
			int32_t len =
				xcb_randr_get_screen_resources_outputs_length(resources);
			outputs = malloc(sizeof(*outputs) * (len > 0 ? len : 1));
			if (outputs && len > 0) {
				memcpy(outputs,
					   xcb_randr_get_screen_resources_outputs(resources),
					   sizeof(*outputs) * len);
				outputs_len = len;
			}
			free(resources);
			resources_pending = false;
		}
	} else if (info_pending) {
		xcb_randr_get_output_info_reply_t *info =
			xcb_randr_get_output_info_reply(conn, info_cookie, NULL);
		if (info) {
			int32_t len = xcb_randr_get_output_info_name_length(info);
			const char *name =
				(const char *)xcb_randr_get_output_info_name(info);
			if (name && len > 0 && cache_len < XWL_CACHE_MAX) {
				int32_t copy = len < XWL_NAME_MAX - 1 ? len : XWL_NAME_MAX - 1;
				memcpy(cache_name[cache_len], name, copy);
				cache_name[cache_len][copy] = '\0';
				cache_output[cache_len] = outputs[outputs_idx];
				cache_len++;
			}
			free(info);
			info_pending = false;
			outputs_idx++;
		}
	}

	if (resources_pending || info_pending) {
		return 0;
	}
	if (outputs_idx < outputs_len) {
		info_pending = true;
		info_cookie = xcb_randr_get_output_info(conn, outputs[outputs_idx],
												XCB_CURRENT_TIME);
		xcb_flush(conn);
		return 0;
	}

	free(outputs);
	outputs = NULL;
	outputs_len = outputs_idx = 0;
	cache_valid = true;
	xwayland_primary_apply();

	/* XWayland announces its outputs shortly after the ready event. */
	for (int32_t i = 0; i < cache_len; i++) {
		if (strncmp(cache_name[i], target_name, XWL_NAME_MAX) == 0) {
			return 0;
		}
	}
	if (retry_count++ < 3 && retry_timer) {
		wl_event_source_timer_update(retry_timer, 200);
	}
	return 0;
}

void xwayland_primary_init(void) {
	xwayland_primary_set(server.selected_monitor);
}

void xwayland_primary_set(Monitor *m) {
	const char *display =
		server.xwayland ? server.xwayland->display_name : NULL;
	if (!display || !m || !m->wlr_output || !m->wlr_output->name) {
		return;
	}

	if (strncmp(display_name, display, XWL_NAME_MAX) != 0) {
		xwayland_primary_close();
		strncpy(display_name, display, XWL_NAME_MAX - 1);
		display_name[XWL_NAME_MAX - 1] = '\0';
	}
	strncpy(target_name, m->wlr_output->name, XWL_NAME_MAX - 1);
	target_name[XWL_NAME_MAX - 1] = '\0';
	retry_count = 0;

	if (!retry_timer) {
		retry_timer =
			wl_event_loop_add_timer(wl_display_get_event_loop(server.display),
									xwayland_primary_retry, NULL);
	}

	xwayland_primary_apply();

	if (conn && !conn_source) {
		conn_source = wl_event_loop_add_fd(
			wl_display_get_event_loop(server.display),
			xcb_get_file_descriptor(conn), WL_EVENT_READABLE,
			xwayland_primary_ready, NULL);
	}
}

#endif
