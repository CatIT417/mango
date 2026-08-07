typedef struct {
	float x, y, w, h;
} OvPlacedRect;

typedef struct {
	float x, y;
} OvPoint;

typedef struct {
	Client *c;
	float orig_w;
	float orig_h;
	float area;
} OvLayoutItem;

static int compare_layout_items(const void *a, const void *b) {
	float area_a = ((const OvLayoutItem *)a)->area;
	float area_b = ((const OvLayoutItem *)b)->area;
	if (area_a < area_b)
		return 1;
	if (area_a > area_b)
		return -1;
	return 0;
}

static bool try_place(OvPlacedRect *placed, int placed_cnt, float w, float h,
					  float gap, float avail_w, float avail_h,
					  OvPlacedRect *out, OvPoint *cands, OvPoint *feas) {
	int cand_cnt = 0;
	cands[cand_cnt++] = (OvPoint){0.0f, 0.0f};

	for (int i = 0; i < placed_cnt; i++) {
		OvPlacedRect p = placed[i];
		cands[cand_cnt++] = (OvPoint){p.x + p.w + gap, p.y};
		cands[cand_cnt++] = (OvPoint){p.x, p.y + p.h + gap};
		cands[cand_cnt++] = (OvPoint){p.x + p.w + gap, p.y + p.h + gap};
	}

	int unique_cnt = 0;
	for (int i = 0; i < cand_cnt; i++) {
		bool dup = false;
		for (int j = 0; j < unique_cnt; j++) {
			if (fabs(cands[i].x - cands[j].x) < 0.5f &&
				fabs(cands[i].y - cands[j].y) < 0.5f) {
				dup = true;
				break;
			}
		}
		if (!dup)
			cands[unique_cnt++] = cands[i];
	}
	cand_cnt = unique_cnt;

	int feas_cnt = 0;
	for (int i = 0; i < cand_cnt; i++) {
		float cx = cands[i].x;
		float cy = cands[i].y;

		if (cx < 0 || cy < 0 || cx + w > avail_w || cy + h > avail_h)
			continue;

		bool overlap = false;
		for (int j = 0; j < placed_cnt; j++) {
			OvPlacedRect p = placed[j];
			if (!(cx + w + gap <= p.x || cx >= p.x + p.w + gap ||
				  cy + h + gap <= p.y || cy >= p.y + p.h + gap)) {
				overlap = true;
				break;
			}
		}
		if (!overlap) {
			feas[feas_cnt++] = (OvPoint){cx, cy};
		}
	}

	if (feas_cnt == 0)
		return false;

	int best = 0;
	for (int i = 1; i < feas_cnt; i++) {
		if (feas[i].y < feas[best].y ||
			(fabs(feas[i].y - feas[best].y) < 0.5f &&
			 feas[i].x < feas[best].x)) {
			best = i;
		}
	}

	out->x = feas[best].x;
	out->y = feas[best].y;
	out->w = w;
	out->h = h;
	return true;
}

void overview_scale(Monitor *m) {
	int32_t target_gappo = config.overviewgappo;
	int32_t target_gappi = config.overviewgappi;

	int orig_n = m->visible_clients;
	if (orig_n == 0)
		return;

	OvLayoutItem *items = calloc(orig_n, sizeof(OvLayoutItem));
	if (!items)
		return;

	int n = 0;
	Client *c;
	wl_list_for_each(c, &clients, link) {
		if (c->mon != m)
			continue;
		if (VISIBLEON(c, m) && !c->isunglobal && !client_is_x11_popup(c)) {
			items[n].c = c;
			float w = c->overview_backup_geom.width;
			float h = c->overview_backup_geom.height;
			if (w <= 0 || h <= 0) {
				w = 100.0f;
				h = 100.0f;
			}
			items[n].orig_w = w;
			items[n].orig_h = h;
			items[n].area = w * h;
			n++;
		}
	}

	if (n == 0) {
		free(items);
		return;
	}

	qsort(items, n, sizeof(OvLayoutItem), compare_layout_items);

	float max_avail_w = fmaxf(1.0f, m->w.width - 2 * target_gappo);
	float max_avail_h = fmaxf(1.0f, m->w.height - 2 * target_gappo);

	int max_points = 1 + 3 * n;
	OvPlacedRect *placed = calloc(n, sizeof(OvPlacedRect));
	OvPoint *cands = calloc(max_points, sizeof(OvPoint));
	OvPoint *feas = calloc(max_points, sizeof(OvPoint));

	if (!placed || !cands || !feas) {
		free(items);
		free(placed);
		free(cands);
		free(feas);
		return;
	}

	float low = 0.0f, high = 1.0f, best_s = 0.0f;
	for (int iter = 0; iter < 50; iter++) {
		float mid = (low + high) / 2.0f;
		bool ok = true;
		int placed_cnt = 0;

		for (int k = 0; k < n; k++) {
			float w = items[k].orig_w * mid;
			float h = items[k].orig_h * mid;
			OvPlacedRect out;
			if (!try_place(placed, placed_cnt, w, h, (float)target_gappi,
						   max_avail_w, max_avail_h, &out, cands, feas)) {
				ok = false;
				break;
			}
			placed[placed_cnt++] = out;
		}

		if (ok) {
			best_s = mid;
			low = mid;
		} else {
			high = mid;
		}
	}

	if (best_s > 0.0f) {
		int placed_cnt = 0;

		for (int k = 0; k < n; k++) {
			float w = items[k].orig_w * best_s;
			float h = items[k].orig_h * best_s;
			OvPlacedRect out;
			try_place(placed, placed_cnt, w, h, (float)target_gappi,
					  max_avail_w, max_avail_h, &out, cands, feas);
			placed[placed_cnt++] = out;
		}

		if (n > 1) {
			float grid_box_w = 0;
			for (int k = 0; k < n - 1; k++) {
				float r = placed[k].x + placed[k].w;
				if (r > grid_box_w)
					grid_box_w = r;
			}

			OvPlacedRect *last = &placed[n - 1];
			float max_x = grid_box_w - last->w;

			if (max_x > last->x) {
				for (int k = 0; k < n - 1; k++) {
					OvPlacedRect p = placed[k];
					if (!(last->y + last->h + target_gappi <= p.y ||
						  last->y >= p.y + p.h + target_gappi)) {
						if (p.x > last->x) {
							float limit = p.x - target_gappi - last->w;
							if (limit < max_x) {
								max_x = limit;
							}
						}
					}
				}

				if (max_x > last->x) {
					last->x += (max_x - last->x) / 2.0f;
				}
			}
		}

		float box_w = 0, box_h = 0;
		for (int k = 0; k < n; k++) {
			float r = placed[k].x + placed[k].w;
			float b = placed[k].y + placed[k].h;
			if (r > box_w)
				box_w = r;
			if (b > box_h)
				box_h = b;
		}

		float dx = (max_avail_w - box_w) / 2.0f;
		float dy = (max_avail_h - box_h) / 2.0f;
		float base_x = m->w.x + target_gappo + dx;
		float base_y = m->w.y + target_gappo + dy;

		// 收集所有客户端的目标几何，最后统一调用 client_tile_resize
		struct wlr_box *overview_boxes = calloc(n, sizeof(*overview_boxes));
		if (!overview_boxes) {
			free(items);
			free(placed);
			free(cands);
			free(feas);
			return;
		}
		for (int k = 0; k < n; k++) {
			float w = items[k].orig_w * best_s;
			float h = items[k].orig_h * best_s;
			int ix = (int)(base_x + placed[k].x + 0.5f);
			int iy = (int)(base_y + placed[k].y + 0.5f);
			int iw = (int)(ix + w + 0.5f) - ix;
			int ih = (int)(iy + h + 0.5f) - iy;
			overview_boxes[k] = (struct wlr_box){ix, iy, iw, ih};
		}

		for (int k = 0; k < n; k++) {
			client_tile_resize(items[k].c, overview_boxes[k], 0);
		}
		free(overview_boxes);
	}

	free(items);
	free(placed);
	free(cands);
	free(feas);
}

void create_jump_hints(Monitor *m) {
	// 未配置 jump_labels 时使用静态默认序列
	const char *jump_labels =
		config.jump_labels ? config.jump_labels : default_jump_labels;
	if (!jump_labels || !jump_labels[0])
		return;
	size_t jump_labels_len = strlen(jump_labels);
	int label_idx = 0;
	Client *c;

	wl_list_for_each(c, &clients, link) {
		if (VISIBLEON(c, m) && !c->isunglobal && !client_is_x11_popup(c)) {
			if (label_idx >= (int)jump_labels_len)
				break;
			char c_char = jump_labels[label_idx];
			c->jump_char = c_char;

			char label_text[2] = {c_char, '\0'};
			mango_jump_label_node_update(c->jump_label_node, label_text,
										 m->wlr_output->scale);
			wlr_scene_node_set_enabled(&c->jump_label_node->scene_buffer->node,
									   true);
			wlr_scene_node_raise_to_top(
				&c->jump_label_node->scene_buffer->node);
			wlr_scene_node_set_position(
				&c->jump_label_node->scene_buffer->node,
				c->geom.width / 2 - c->jump_label_node->logical_width / 2,
				c->geom.height / 2 - c->jump_label_node->logical_height / 2);
			label_idx++;
		}
	}
}

void begin_jump_mode(Monitor *m) { m->is_jump_mode = 1; }

void finish_jump_mode(Monitor *m) {
	if (!m->is_jump_mode)
		return;

	Client *c;
	wl_list_for_each(c, &clients, link) {
		if (VISIBLEON(c, m)) {
			if (c->jump_label_node->scene_buffer->node.enabled) {
				c->jump_char = '\0';
				wlr_scene_node_set_enabled(
					&c->jump_label_node->scene_buffer->node, false);
			}
		}
	}
	m->is_jump_mode = 0;
}

void overview(Monitor *m) {

	overview_scale(m);

	if (m->is_jump_mode) {
		create_jump_hints(m);
	}
}
