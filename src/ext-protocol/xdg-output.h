/* 自定义 xdg-output：xwayland_ignore_scale 时给 XWayland 发物理坐标/尺寸 */
#include <math.h>

#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "xdg-output-unstable-v1-protocol.h"

#define MANGO_XDG_OUTPUT_MANAGER_VERSION 3
#define MANGO_XDG_OUTPUT_DONE_DEPRECATED_SINCE_VERSION 3

struct MangoXDGOutput {
	struct wl_resource *resource;
	struct wl_list link; /* xdg_output_resources */
	struct wlr_output *wlr_output;
	bool is_xwayland;
};

static struct wl_global *xdg_output_global;
static struct wl_list xdg_output_resources;

static const struct zxdg_output_v1_interface xdg_output_impl;
static const struct zxdg_output_manager_v1_interface xdg_output_manager_impl;

static void xdg_output_update_details(struct MangoXDGOutput *output) {
	if (!output->resource || !output->wlr_output)
		return;

	Monitor *m = output->wlr_output->data;
	if (!m)
		return;

	int32_t x, y, w, h;
	if (output->is_xwayland && config.xwayland_ignore_scale) {
		/* 欺骗 XWayland：mango 把 X11 窗口放在 逻辑×scale 的物理坐标空间，
		 * 因此告诉 XWayland 屏幕原点在 逻辑×scale、矩形为物理尺寸 */
		float scale =
			output->wlr_output->scale > 0.f ? output->wlr_output->scale : 1.f;
		x = (int32_t)roundf(m->m.x * scale);
		y = (int32_t)roundf(m->m.y * scale);
		w = output->wlr_output->width;
		h = output->wlr_output->height;
	} else {
		/* xwayland_ignore_scale=0：与普通客户端一样用逻辑坐标 */
		x = m->m.x;
		y = m->m.y;
		w = m->m.width;
		h = m->m.height;
	}

	zxdg_output_v1_send_logical_position(output->resource, x, y);
	zxdg_output_v1_send_logical_size(output->resource, w, h);

	if (wl_resource_get_version(output->resource) <
		MANGO_XDG_OUTPUT_DONE_DEPRECATED_SINCE_VERSION)
		zxdg_output_v1_send_done(output->resource);
}

static void xdg_output_handle_resource_destroy(struct wl_resource *resource) {
	struct MangoXDGOutput *output = wl_resource_get_user_data(resource);
	if (!output)
		return;
	wl_list_remove(&output->link);
	free(output);
}

static void xdg_output_handle_destroy(struct wl_client *client,
									  struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void xdg_output_manager_handle_destroy(struct wl_client *client,
											  struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void xdg_output_manager_handle_get_xdg_output(
	struct wl_client *client, struct wl_resource *manager_resource, uint32_t id,
	struct wl_resource *output_resource) {
	struct wlr_output *wlr_output = wlr_output_from_resource(output_resource);
	struct MangoXDGOutput *output = ecalloc(1, sizeof(*output));
	if (!output) {
		wl_client_post_no_memory(client);
		return;
	}

	output->resource =
		wl_resource_create(client, &zxdg_output_v1_interface,
						   wl_resource_get_version(manager_resource), id);
	if (!output->resource) {
		wl_client_post_no_memory(client);
		free(output);
		return;
	}

	output->wlr_output = wlr_output;
#ifdef XWAYLAND
	/* XWayland 的 X server 也是一个 wayland 客户端 */
	if (xwayland && xwayland->server && xwayland->server->client == client)
		output->is_xwayland = true;
#endif

	wl_resource_set_implementation(output->resource, &xdg_output_impl, output,
								   xdg_output_handle_resource_destroy);
	wl_list_insert(&xdg_output_resources, &output->link);

	if (!wlr_output)
		return;

	uint32_t version = wl_resource_get_version(output->resource);
	if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION && wlr_output->name)
		zxdg_output_v1_send_name(output->resource, wlr_output->name);
	if (version >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION &&
		wlr_output->description)
		zxdg_output_v1_send_description(output->resource,
										wlr_output->description);

	xdg_output_update_details(output);

	if (wl_resource_get_version(output_resource) >=
			WL_OUTPUT_DONE_SINCE_VERSION &&
		version >= MANGO_XDG_OUTPUT_DONE_DEPRECATED_SINCE_VERSION)
		wl_output_send_done(output_resource);
}

static void xdg_output_manager_bind(struct wl_client *client, void *data,
									uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(
		client, &zxdg_output_manager_v1_interface, version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &xdg_output_manager_impl, NULL,
								   NULL);
}

static const struct zxdg_output_v1_interface xdg_output_impl = {
	.destroy = xdg_output_handle_destroy,
};

static const struct zxdg_output_manager_v1_interface xdg_output_manager_impl = {
	.destroy = xdg_output_manager_handle_destroy,
	.get_xdg_output = xdg_output_manager_handle_get_xdg_output,
};

/* 更新所有客户端的 xdg-output 详情 */
static void xdg_output_update_all(void) {
	struct MangoXDGOutput *output, *tmp;
	wl_list_for_each_safe(output, tmp, &xdg_output_resources, link) {
		xdg_output_update_details(output);
		/* 通知 wl_output.done */
		if (output->wlr_output)
			wlr_output_schedule_done(output->wlr_output);
	}
}

/* 销毁指定输出对应的所有 xdg-output 资源（monitor 移除时调用） */
static void xdg_output_cleanup_output(struct wlr_output *wlr_output) {
	struct MangoXDGOutput *output, *tmp;
	wl_list_for_each_safe(output, tmp, &xdg_output_resources, link) {
		if (output->wlr_output == wlr_output)
			wl_resource_destroy(output->resource);
	}
}

void xdg_output_init(void) {
	wl_list_init(&xdg_output_resources);
	xdg_output_global = wl_global_create(dpy, &zxdg_output_manager_v1_interface,
										 MANGO_XDG_OUTPUT_MANAGER_VERSION, NULL,
										 xdg_output_manager_bind);
	if (!xdg_output_global)
		wlr_log(WLR_ERROR, "failed to create zxdg_output_manager_v1 global");
}
