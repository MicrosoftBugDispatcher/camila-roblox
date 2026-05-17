#pragma once
#include "imgui/imgui.h"
#include <cstdarg>

namespace notify {

enum notify_level_t {
  notify_info,
  notify_success,
  notify_warn,
  notify_error,
};

void print(notify_level_t level, const char* fmt, ...);
void update();
void render();

}
