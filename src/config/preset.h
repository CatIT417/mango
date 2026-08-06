#define MODKEY WLR_MODIFIER_ALT

/* 最大可配置的 tag 数量：决定静态数组大小（Pertag 等），同时受 uint32_t
 * 位宽限制 */
#define tag_num_MAX 31
static const char *tags[] = {
	"1",  "2",	"3",  "4",	"5",  "6",	"7",  "8",	"9",  "10", "11",
	"12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22",
	"23", "24", "25", "26", "27", "28", "29", "30", "31",
};

static const struct xkb_rule_names xkb_fallback_rules = {
	.layout = "us",
	.variant = NULL,
	.model = NULL,
	.rules = NULL,
	.options = NULL,
};
