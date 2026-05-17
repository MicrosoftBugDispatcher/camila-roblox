#pragma once
#include "imgui/imgui.h"

namespace theme {

inline const ImVec4 menu_bg = ImVec4(24.f, 21.f, 28.f, 255.f);
inline const ImVec4 topbar_grad_t = ImVec4(30.f, 26.f, 36.f, 255.f);
inline const ImVec4 topbar_grad_b = ImVec4(22.f, 20.f, 26.f, 255.f);
inline const ImVec4 topbar_inline = ImVec4(26.f, 22.f, 32.f, 255.f);
inline const ImVec4 accent_border = ImVec4(15.f, 13.f, 18.f, 255.f);
inline const ImVec4 menu_text = ImVec4(184.f, 176.f, 192.f, 255.f);
inline const ImVec4 tab_sep_1 = ImVec4(37.f, 32.f, 48.f, 255.f);
inline const ImVec4 tab_selected_indicator = ImVec4(72.f, 64.f, 88.f, 255.f);
inline const ImVec4 inactive_tab_t = ImVec4(30.f, 26.f, 36.f, 255.f);
inline const ImVec4 inactive_tab_b = ImVec4(22.f, 20.f, 26.f, 255.f);

inline const float topbar_h = 25.f;
inline const float tab_bar_h = 25.f;
inline const ImVec2 menu_size = ImVec2(700.f, 550.f);
inline const float menu_expand_dur = 0.8f;
inline const float menu_fade_dur = 0.3f;
inline const float out_thick = 1.f;
inline const int tab_sep_cnt = 2;
inline const float tab_pad = 8.f;
inline const float tab_gap = 4.f;
inline const float tab_speed = 8.f;

inline const ImVec4 notify_info = ImVec4(61.f, 53.f, 72.f, 255.f);
inline const ImVec4 notify_success = ImVec4(21.f, 150.f, 30.f, 255.f);
inline const ImVec4 notify_warn = ImVec4(237.f, 155.f, 64.f, 255.f);
inline const ImVec4 notify_error = ImVec4(214.f, 69.f, 80.f, 255.f);
inline const float notify_duration = 4.f;
inline const float notify_slide_speed = 12.f;
inline const float notify_exit_speed = 14.f;
inline const float notify_spacing = 6.f;
inline const float notify_pad_x = 12.f;
inline const float notify_pad_y = 8.f;

inline ImU32 col_alpha(ImVec4 rgb, int a) {
  return IM_COL32((int)rgb.x, (int)rgb.y, (int)rgb.z, a);
}

}

#include "../widgets/widgets.h"
