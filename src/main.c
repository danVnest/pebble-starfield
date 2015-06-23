#include <pebble.h>

#define STAR_COUNT 40
#define INT_PRECISION 1000
#define X_MAX (144 * INT_PRECISION)
#define Y_MAX (168 * INT_PRECISION)
#define Z_MAX (64 * INT_PRECISION)
#define STAR_SIZE_MAX (2 * INT_PRECISION)
#define FLOW_MAX (64 * INT_PRECISION)
#define PIXELS_MAX 8

enum STORAGE_KEYS {
	STORAGE_SETTINGS
};

enum SETTING_KEYS {
	SETTING_ANIMATE,
	SETTING_KEY_COUNT
};

static uint8_t settings[SETTING_KEY_COUNT];
static Window *window;
static Layer *starfield_layer;
static TextLayer *time_layer;
static GFont font;
static AppTimer *animation_timer;
static AppTimer *end_tap_timer;
static uint32_t delta = 40;
static uint32_t tap_duration = 3000;
static AppSync app_sync;
static uint8_t sync_buffer[32];

struct XYZ {
	int x;
	int y;
	int z;
};
static struct XYZ *flow;
static struct XYZ *desired_flow;
static struct XYZ *flow_acceleration;

struct Star {
	int x;
	int y;
	int z;
	int size;
};
static struct Star *stars[STAR_COUNT];

/**************** Starfield ****************/
static struct Star* create_star() { // TODO: consider moving into create_starfield()
	struct Star *star = malloc(sizeof(struct Star));
	star->x = rand() % X_MAX;
	star->y = rand() % Y_MAX;
	star->z = rand() % Z_MAX;
	star->size = rand() % STAR_SIZE_MAX;
	return star;
}

static void respawn_star(struct Star *star) { // TODO: consider moving into update_starfield()	
	int position = rand() % (X_MAX + Y_MAX);
	if (position <= X_MAX) {
		star->x = position;
		if (flow->y >= 0) star->y = 0;
		else star->y = Y_MAX;
	}
	else {
		if (flow->x >= 0) star->x = 0;
		else star->x = X_MAX;
		star->y = position - X_MAX;		
	}
	star->z = rand() % Z_MAX;
	star->size = rand() % STAR_SIZE_MAX;
}

static void update_starfield() {
	for (int i = 0; i < STAR_COUNT; i++) {
		struct Star *star = stars[i]; // TODO: necessary? check efficiency
		star->x += flow->x * star->z / Z_MAX;
		star->y += flow->y * star->z / Z_MAX;
		if ((star->x > X_MAX) || (star->x < 0) || (star->y > Y_MAX) || (star->y < 0)) respawn_star(star);
	}
	layer_mark_dirty(starfield_layer);
}

static void draw_starfield(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	for(int i = 0; i < STAR_COUNT; i++) {
		struct Star *star = stars[i];
		int apparent_size = star->size * star->z * PIXELS_MAX / STAR_SIZE_MAX / Z_MAX + 1;
		graphics_fill_rect(ctx, GRect(star->x / INT_PRECISION, star->y / INT_PRECISION, apparent_size, apparent_size), 0, GCornerNone);
	}
}

static void create_starfield() {
	flow = malloc(sizeof(struct XYZ));
	flow->x = 0;
	flow->y = 0;
	desired_flow = malloc(sizeof(struct XYZ));
	// TOOD: proper startup animation
	desired_flow->x = rand() % FLOW_MAX - FLOW_MAX / 2;
	desired_flow->y = rand() % FLOW_MAX - FLOW_MAX / 2;
	flow_acceleration = malloc(sizeof(struct XYZ));
	flow_acceleration->x = desired_flow->x * (int)delta / 2000;
	flow_acceleration->y = desired_flow->y * (int)delta / 2000;
	for (int i = 0; i < STAR_COUNT; i++) stars[i] = create_star();
}

static void destroy_starfield() {
	for (int i = 0; i < STAR_COUNT; i++) free(stars[i]);
	free(flow);
	free(desired_flow);
	free(flow_acceleration);
}

/**************** Time ****************/
static void update_time(struct tm *tick_time) {
	static char buffer[] = "00:00";
	if(clock_is_24h_style() == true) strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
	else strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
	text_layer_set_text(time_layer, buffer);
}

static void animate(void *data) {
	flow->x += flow_acceleration->x;
	if (((flow_acceleration->x > 0) && (flow->x > desired_flow->x)) || ((flow_acceleration->x < 0) && (flow->x < desired_flow->x))) {
		flow->x = desired_flow->x;
		flow_acceleration->x = 0;
	}
	flow->y += flow_acceleration->y;
	if (((flow_acceleration->y > 0) && (flow->y > desired_flow->y)) || ((flow_acceleration->y < 0) && (flow->y < desired_flow->y))) {
		flow->y = desired_flow->y;
		flow_acceleration->y = 0;
	}
	if ((flow->x != 0) || (flow->y != 0)) {
		update_starfield();
		animation_timer = app_timer_register(delta, animate, 0);
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time(tick_time);
}

/**************** User Input ****************/
static void end_tap_handler(void *data) { // TODO: put function into animate()?
	if (settings[SETTING_ANIMATE] == 2) {
		desired_flow->x = rand() % FLOW_MAX - FLOW_MAX / 2;
		desired_flow->y = rand() % FLOW_MAX - FLOW_MAX / 2;
		end_tap_timer = app_timer_register(tap_duration, end_tap_handler, 0);
	}
	else {
		desired_flow->x = 0;
		desired_flow->y = 0;
		end_tap_timer = NULL;		
	}
	flow_acceleration->x = (desired_flow->x - flow->x) * (int)delta / 2000;
	flow_acceleration->y = (desired_flow->y - flow->y) * (int)delta / 2000;
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
	desired_flow->x = rand() % FLOW_MAX - FLOW_MAX / 2;
	desired_flow->y = rand() % FLOW_MAX - FLOW_MAX / 2;
	flow_acceleration->x = (desired_flow->x - flow->x) * (int)delta / 2000;
	flow_acceleration->y = (desired_flow->y - flow->y) * (int)delta / 2000;
	if (end_tap_timer == NULL) {
		end_tap_timer = app_timer_register(tap_duration, end_tap_handler, 0);
		if ((flow->x == 0) && (flow->y == 0)) animation_timer = app_timer_register(delta, animate, 0);
	}
	else app_timer_reschedule(end_tap_timer, tap_duration);
}

/**************** App Communication ****************/
static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
	settings[key] = new_tuple->value->int8;
	animation_timer = app_timer_register(delta, animate, 0);
	end_tap_timer = app_timer_register(0, end_tap_handler, 0);
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "sync error!");
}

/**************** Window ****************/
static void window_load(Window *window) {
	window_set_background_color(window, GColorBlack);
	starfield_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(starfield_layer, draw_starfield);
	layer_add_child(window_get_root_layer(window), starfield_layer);
	time_layer = text_layer_create(GRect(0, 55, 144, 50));
	font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_38));
	text_layer_set_font(time_layer, font);
	text_layer_set_text_color(time_layer, GColorWhite);
	text_layer_set_background_color(time_layer, GColorClear);
	text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
	time_t raw_time = time(NULL);
	struct tm *tick_time = localtime(&raw_time);
	update_time(tick_time);
}

static void window_unload(Window *window) {
	layer_destroy(starfield_layer);
	text_layer_destroy(time_layer);
	fonts_unload_custom_font(font);
}

/**************** Main ****************/
static void init() {
	if (persist_exists(STORAGE_SETTINGS)) persist_read_data(STORAGE_SETTINGS, settings, persist_get_size(STORAGE_SETTINGS));
	else {
		settings[SETTING_ANIMATE] = 1;
	}
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	create_starfield();
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	if (settings[SETTING_ANIMATE] != 0) {
		accel_tap_service_subscribe(tap_handler);
		animation_timer = app_timer_register(delta, animate, 0);
		end_tap_timer = app_timer_register(tap_duration, end_tap_handler, 0);
	}
	window_stack_push(window, true);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	Tuplet settings_sync[] = {
		TupletInteger(SETTING_ANIMATE, settings[SETTING_ANIMATE])
	};
	app_sync_init(&app_sync, sync_buffer, sizeof(sync_buffer), settings_sync, ARRAY_LENGTH(settings_sync), sync_changed_handler, sync_error_handler, NULL);
}

static void deinit() {
	destroy_starfield();
	tick_timer_service_unsubscribe();
	window_destroy(window);
	app_sync_deinit(&app_sync);
	persist_write_data(STORAGE_SETTINGS, settings, sizeof(settings));
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}