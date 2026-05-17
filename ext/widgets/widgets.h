#pragma once
#include "imgui/imgui.h"

namespace widgets {

bool begin_combo(const char* label, const char* preview, const char* const* items,
    int item_count, int& current_idx);
bool slider_scalar(const char* label, float& value, float v_min, float v_max,
    const char* fmt = "%.0f%%");
bool slider_scalar(const char* label, int& value, int v_min, int v_max,
    const char* fmt = "%d");
bool checkbox(const char* label, bool& value);
bool checkbox_color(const char* label, bool& value, float color[4]);
bool checkbox_keybind(const char* label, bool& value, int& key);
bool button(const char* label, bool active = false);
bool multi_button(const char** labels, int count, int& selected);
bool color_edit(const char* label, float color[4]);
bool keybind(const char* label, int& key);
const char* get_key_name(int vk);
void keybind_list(float x, float y, float width, const char** labels,
    const int* keys, int count, float alpha = 1.f);
bool section_begin(const char* label, const char** tab_labels = nullptr,
    int tab_count = 0, int* tab_selected = nullptr, float height = -1.f);
void section_end();

}
