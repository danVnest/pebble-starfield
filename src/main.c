#include <pebble.h>

static Window *window;
static TextLayer *time_layer;

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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

/**************** Window ****************/
static void window_load(Window *window) {
    window_set_background_color(window, GColorBlack);
    time_layer = text_layer_create(GRect(0, 55, 144, 50));
    text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_38)));
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
    time_t raw_time = time(NULL);
    struct tm *tick_time = localtime(&raw_time);
    update_time(tick_time);
}

static void window_unload(Window *window) {
    text_layer_destroy(time_layer);
}

/**************** Main ****************/
static void init() {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    window_stack_push(window, true);
}

static void deinit() {
    tick_timer_service_unsubscribe();
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}