#include <pebble.h>
#include "settings.h"
#include "main.h"

static uint8_t settings[SETTING_KEY_COUNT];
static AppSync app_sync;
static uint8_t sync_buffer[32];

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
	if (settings[key] != new_tuple->value->uint8) {
		settings[key] = new_tuple->value->uint8;
		restart();
	}
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "sync error!");
}

uint8_t get_setting(int key) {
	return settings[key];
}
	
void sync_settings() {
	size_t stored_byte_count = persist_get_size(STORAGE_SETTINGS);
	if (stored_byte_count == sizeof(settings)) persist_read_data(STORAGE_SETTINGS, settings, stored_byte_count);
	else settings[SETTING_ANIMATE] = 1;
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	Tuplet settings_sync[] = { TupletInteger(SETTING_ANIMATE, settings[SETTING_ANIMATE]) };
	app_sync_init(&app_sync, sync_buffer, sizeof(sync_buffer), settings_sync, ARRAY_LENGTH(settings_sync), sync_changed_handler, sync_error_handler, NULL);
}

void save_settings() {
	persist_write_data(STORAGE_SETTINGS, settings, sizeof(settings));
	app_sync_deinit(&app_sync);
}