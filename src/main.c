#include <pebble.h>

#define STAR_COUNT 40
    
static Window *window;
static Layer *starfield_layer;
static TextLayer *time_layer;
static AppTimer *timer;
static uint32_t delta = 40;

struct Star {
    short x;
    short y;
    short z;
};

struct Star *stars[STAR_COUNT];

/**************** Starfield ****************/
static struct Star* create_star() { // TODO: consider moving into create_starfield()
    struct Star *star = malloc(sizeof(struct Star));
	star->x = rand() % 144;
	star->y = rand() % 168;
	star->z = rand() % 6 + 1;
	return star;
}

static void create_starfield() {
    for (int i = 0; i < STAR_COUNT; i++) stars[i] = create_star();
}

static void destroy_starfield() {
    for (int i = 0; i < STAR_COUNT; i++) free(stars[i]);
}

static void respawn_star(struct Star *star) { // TODO: consider moving into update_starfield()
	star->x = 0;
	star->y = rand() % 168;
	star->z = rand() % 6 + 1;
}

static void update_starfield() {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i]->x += stars[i]->z / 2 + 1;
        if (stars[i]->x > 144) respawn_star(stars[i]);
    }
    layer_mark_dirty(starfield_layer);
}

static void draw_starfield(Layer *layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    for(int i = 0; i < STAR_COUNT; i++) {
        graphics_fill_rect(ctx, GRect(stars[i]->x, stars[i]->y, stars[i]->z, stars[i]->z), 0, GCornerNone);
    }
}

/**************** Time ****************/
static void update_time(struct tm *tick_time) {
    static char buffer[] = "00:00";
    if(clock_is_24h_style() == true) {
        strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
        strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }
    text_layer_set_text(time_layer, buffer);
}

static void timer_callback(void *data) {
    update_starfield();
    timer = app_timer_register(delta, timer_callback, 0);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

/**************** Window ****************/
static void window_load(Window *window) {
    window_set_background_color(window, GColorBlack);
    starfield_layer = layer_create(GRect(0, 0, 144, 168));
    layer_set_update_proc(starfield_layer, draw_starfield);
    layer_add_child(window_get_root_layer(window), starfield_layer);
    time_layer = text_layer_create(GRect(0, 55, 144, 50));
    text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_38)));
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
    time_t raw_time = time(NULL);
    struct tm *tick_time = localtime(&raw_time);
    update_time(tick_time);
    timer = app_timer_register(delta, timer_callback, 0);
}

static void window_unload(Window *window) {
    layer_destroy(starfield_layer);
    text_layer_destroy(time_layer);
}

/**************** Main ****************/
static void init() {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    create_starfield();
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    window_stack_push(window, true);
}

static void deinit() {
    destroy_starfield();
    tick_timer_service_unsubscribe();
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}