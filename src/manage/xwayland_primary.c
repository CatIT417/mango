#include "mango/manage/xwayland_primary.h"

#include "mango/common/server.h"
#include "mango/manage/monitor.h"

#ifdef XWAYLAND
#include <stdlib.h>
#include <string.h>
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
static char applied_name[XWL_NAME_MAX];

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
static bool cache_refresh_attempted = false;

static bool xwayland_server_running(void) {
	return server.xwayland && server.xwayland->server &&
		   server.xwayland->server->ready;
}

static int32_t xwayland_primary_ready(int32_t fd, uint32_t mask, void *data);

static void xwayland_primary_unwatch(void) {
	if (conn_source) {
		wl_event_source_remove(conn_source);
		conn_source = NULL;
	}
}

static void xwayland_primary_watch(void) {
	if (!conn || conn_source) {
		return;
	}
	conn_source =
		wl_event_loop_add_fd(wl_display_get_event_loop(server.display),
							 xcb_get_file_descriptor(conn), WL_EVENT_READABLE,
							 xwayland_primary_ready, NULL);
}

static void xwayland_primary_close(void) {
	xwayland_primary_unwatch();
	if (conn) {
		xcb_disconnect(conn);
		conn = NULL;
	}
	root = XCB_NONE;
	applied_name[0] = '\0';
	resources_pending = false;
	info_pending = false;
	free(outputs);
	outputs = NULL;
	outputs_len = outputs_idx = 0;
	cache_len = 0;
	cache_valid = false;
}

/* A connection of our own is an X client, and Xwayland exits once the last
 * client disconnects when it is not persistent. Release it in that mode, keep
 * it otherwise so RandR changes stay watchable. */
static void xwayland_primary_release(void) {
	if (config.xwayland_persistence) {
		return;
	}
	/* Round trip so a request still in flight is taken before the connection
	 * goes away, a plain flush can be lost with the disconnect. */
	if (conn) {
		free(xcb_randr_get_output_primary_reply(
			conn, xcb_randr_get_output_primary(conn, root), NULL));
	}
	xwayland_primary_close();
}

static void xwayland_primary_apply(void);
static void xwayland_primary_start(void);
static int32_t xwayland_primary_ready(int32_t fd, uint32_t mask, void *data);

static bool xwayland_primary_connect(void) {
	if (conn) {
		return true;
	}

	int32_t screen_num = 0;
	conn = xcb_connect(display_name, &screen_num);
	if (!conn || xcb_connection_has_error(conn) ||
		xcb_get_setup(conn) == NULL) {
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

	/* Watch RandR screen changes: whichever way the primary output is touched
	 * afterwards (a client, a mode change, a hotplug event) it has to end up
	 * back on the monitor the rules select. */
	const xcb_query_extension_reply_t *randr =
		xcb_get_extension_data(conn, &xcb_randr_id);
	if (randr && randr->present) {
		xcb_randr_select_input(conn, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	}
	xcb_flush(conn);

	cache_valid = false;
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
	if (!conn) {
		return;
	}
	if (!target_name[0]) {
		xwayland_primary_close();
		return;
	}
	if (resources_pending || info_pending) {
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
		strncpy(applied_name, target_name, XWL_NAME_MAX - 1);
		applied_name[XWL_NAME_MAX - 1] = '\0';
		cache_refresh_attempted = false;
		xwayland_primary_release();
		return;
	}

	if (!cache_refresh_attempted) {
		cache_refresh_attempted = true;
		cache_valid = false;
		xwayland_primary_build_cache();
		return;
	}
	/* The output is not there (yet). Keep watching instead of giving up, the
	 * next RandR change retries with a fresh cache. */
	applied_name[0] = '\0';
	xwayland_primary_release();
}

static void xwayland_primary_start(void) {
	if (strncmp(applied_name, target_name, XWL_NAME_MAX) == 0) {
		return;
	}
	if (!xwayland_server_running()) {
		applied_name[0] = '\0';
		cache_valid = false;
		return;
	}
	if (conn && xcb_connection_has_error(conn)) {
		xwayland_primary_close();
	}
	if (!conn && !xwayland_primary_connect()) {
		return;
	}

	xwayland_primary_watch();
	xwayland_primary_apply();
}

static int32_t xwayland_primary_ready(int32_t fd, uint32_t mask, void *data) {
	if (!conn || (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR))) {
		xwayland_primary_close();
		return 0;
	}

	xcb_generic_event_t *event;
	while ((event = xcb_poll_for_event(conn)) != NULL) {
		free(event);
	}

	if (resources_pending) {
		xcb_randr_get_screen_resources_reply_t *resources =
			xcb_randr_get_screen_resources_reply(conn, resources_cookie, NULL);
		if (!resources) {
			xwayland_primary_close();
			return 0;
		}

		int32_t len = xcb_randr_get_screen_resources_outputs_length(resources);
		outputs = malloc(sizeof(*outputs) * (len > 0 ? len : 1));
		if (outputs && len > 0) {
			memcpy(outputs, xcb_randr_get_screen_resources_outputs(resources),
				   sizeof(*outputs) * len);
			outputs_len = len;
		}
		free(resources);
		resources_pending = false;
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
		}
		info_pending = false;
		outputs_idx++;
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
	/* Also the re-assert path: Xwayland tells us about screen changes here and
	 * the primary is put back on the configured output. Setting it to the
	 * value it already has is a no-op, so this cannot loop. */
	xwayland_primary_apply();
	return 0;
}

/* Monitor the monitor rules designate as the X11 primary output. Falls back to
 * the monitor the earliest rule matches, or the first enabled one, so X11
 * clients always have a stable origin. */
static Monitor *xwayland_primary_rule_monitor(void) {
	Monitor *m = NULL, *first = NULL, *by_rule = NULL;
	int32_t best_rule = INT32_MAX;

	/* The list is head-inserted, walk it backwards to start at the monitor
	 * that was connected first. */
	wl_list_for_each_reverse(m, &server.monitors, link) {
		int32_t i;

		if (!m->wlr_output || !m->wlr_output->enabled) {
			continue;
		}
		if (!first) {
			first = m;
		}

		/* Only the first matching rule applies to a monitor. */
		for (i = 0; i < config.monitor_rules_count; i++) {
			if (monitor_matches_rule(m, &config.monitor_rules[i])) {
				break;
			}
		}
		if (i >= config.monitor_rules_count) {
			continue;
		}
		if (config.monitor_rules[i].primary) {
			return m;
		}
		if (i < best_rule) {
			best_rule = i;
			by_rule = m;
		}
	}

	return by_rule ? by_rule : first;
}

static void xwayland_primary_set(Monitor *m) {
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
	if (strncmp(target_name, m->wlr_output->name, XWL_NAME_MAX) != 0) {
		cache_refresh_attempted = false;
	}
	strncpy(target_name, m->wlr_output->name, XWL_NAME_MAX - 1);
	target_name[XWL_NAME_MAX - 1] = '\0';

	if (strncmp(applied_name, target_name, XWL_NAME_MAX) == 0) {
		return;
	}

	xwayland_primary_start();
}

void xwayland_primary_init(void) {
	const char *display =
		server.xwayland ? server.xwayland->display_name : NULL;
	if (!display) {
		return;
	}

	xwayland_primary_close();
	strncpy(display_name, display, XWL_NAME_MAX - 1);
	display_name[XWL_NAME_MAX - 1] = '\0';

	xwayland_primary_update();
}

/* Points the X11 primary output at the monitor the rules select. The primary
 * output is only ever driven by the rules, never by focus or pointer position,
 * so clients keep a stable coordinate origin for the whole session. */
void xwayland_primary_update(void) {
	cache_refresh_attempted = false;
	applied_name[0] = '\0';
	cache_valid = false;

	Monitor *m = xwayland_primary_rule_monitor();
	if (m) {
		xwayland_primary_set(m);
	} else {
		target_name[0] = '\0';
	}
}

#endif
