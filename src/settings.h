#pragma once

enum STORAGE_KEYS {
	STORAGE_SETTINGS
};
enum SETTING_KEYS {
	SETTING_ANIMATE,
	SETTING_KEY_COUNT
};

uint8_t get_setting(int key);
void sync_settings();
void save_settings();