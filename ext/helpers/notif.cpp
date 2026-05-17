#include "notif.h"
#include "helper.h"
#include "imgui/imgui_internal.h"
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>

namespace notify {

static constexpr ImU32 c_bg_t = IM_COL32(32, 32, 32, 255);
static constexpr ImU32 c_bg_b = IM_COL32(24, 24, 24, 255);
static constexpr ImU32 c_wbg = IM_COL32(36, 36, 36, 255);
static constexpr ImU32 c_outline = IM_COL32(0, 0, 0, 255);
static constexpr ImU32 c_inline = IM_COL32(50, 50, 50, 255);
static constexpr ImU32 c_txt = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 c_acc = IM_COL32(244, 180, 245, 255);
static constexpr ImU32 c_acc_d = IM_COL32(170, 110, 171, 255);

struct entry_t {
  std::string text;
  notify_level_t type;
  float timer;
  float slide_t;
  float alpha;
  bool exiting;
};

static std::vector<entry_t> s_entries;

static ImU32 level_to_color(notify_level_t type) {
  switch (type) {
  case notify_info:    return theme::col_alpha(theme::notify_info, 255);
  case notify_success: return theme::col_alpha(theme::notify_success, 255);
  case notify_warn:    return theme::col_alpha(theme::notify_warn, 255);
  case notify_error:   return theme::col_alpha(theme::notify_error, 255);
  default:             return theme::col_alpha(theme::notify_info, 255);
  }
}

static const char* level_to_label(notify_level_t type) {
  switch (type) {
  case notify_info:    return "Info";
  case notify_success: return "Success";
  case notify_warn:    return "Warning";
  case notify_error:   return "Error";
  default:             return "Info";
  }
}

static ImU32 col_lerp(ImU32 a, ImU32 b, float t) {
  ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
  ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
  float rx = ca.x + (cb.x - ca.x) * t;
  float ry = ca.y + (cb.y - ca.y) * t;
  float rz = ca.z + (cb.z - ca.z) * t;
  float rw = ca.w + (cb.w - ca.w) * t;
  return ImGui::ColorConvertFloat4ToU32(ImVec4(rx, ry, rz, rw));
}

static ImU32 col_alpha(ImU32 c, float a) {
  ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
  v.w *= a;
  return ImGui::ColorConvertFloat4ToU32(v);
}

static float anim_tick(float& t, float target, float speed, float dt) {
  t += (target - t) * (1.f - std::exp(-speed * dt));
  if (t < 0.0001f) t = 0.f;
  if (t > 0.999f) t = 1.f;
  return t;
}

static void draw_combo_border(ImDrawList* dl, ImVec2 min_pt, ImVec2 max_pt, float alpha = 1.f) {
  ImU32 out_c = alpha >= 1.f ? c_outline : col_alpha(c_outline, alpha);
  ImU32 in_c = alpha >= 1.f ? c_inline : col_alpha(c_inline, alpha);
  dl->AddRect(min_pt, max_pt, out_c, 0.f, 0, theme::out_thick);
  dl->AddRect(ImVec2(min_pt.x + 1.f, min_pt.y + 1.f),
      ImVec2(max_pt.x - 1.f, max_pt.y - 1.f), in_c, 0.f, 0, theme::out_thick);
}

void print(notify_level_t level, const char* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  entry_t e;
  e.text = buf;
  e.type = level;
  e.timer = theme::notify_duration;
  e.slide_t = 0.f;
  e.alpha = 0.f;
  e.exiting = false;
  s_entries.push_back(e);
}

void update() {
  float dt = ImGui::GetIO().DeltaTime;
  for (size_t i = 0; i < s_entries.size(); ) {
    entry_t& e = s_entries[i];
    if (e.exiting) {
      anim_tick(e.slide_t, 0.f, theme::notify_exit_speed, dt);
      anim_tick(e.alpha, 0.f, theme::notify_exit_speed, dt);
      if (e.slide_t < 0.001f) {
        s_entries.erase(s_entries.begin() + (ptrdiff_t)i);
        continue;
      }
    } else {
      anim_tick(e.slide_t, 1.f, theme::notify_slide_speed, dt);
      anim_tick(e.alpha, 1.f, theme::notify_slide_speed, dt);
      e.timer -= dt;
      if (e.timer <= 0.f)
        e.exiting = true;
    }
    ++i;
  }
}

void render() {
  if (s_entries.empty()) return;

  const ImGuiIO& io = ImGui::GetIO();
  const float spacing = theme::notify_spacing;
  const float pad_x = theme::notify_pad_x;
  const float pad_y = theme::notify_pad_y;
  const float bar_h = 4.f;
  const float slide_off = 100.f;
  const float strip_w = 4.f;
  const float label_gap = 2.f;

  float pos_y = io.DisplaySize.y - spacing;

  for (int idx = (int)s_entries.size() - 1; idx >= 0; idx--) {
    entry_t& e = s_entries[idx];
    const char* str = e.text.c_str();
    const char* label_str = level_to_label(e.type);
    ImVec2 text_sz = ImGui::CalcTextSize(str);
    ImVec2 label_sz = ImGui::CalcTextSize(label_str);
    float content_w = (text_sz.x > label_sz.x ? text_sz.x : label_sz.x) + pad_x * 2.f + strip_w;
    float content_h = label_sz.y + label_gap + text_sz.y + pad_y * 2.f + bar_h;
    float box_w = content_w;
    float box_h = content_h;
    float target_x = io.DisplaySize.x - box_w - spacing;
    float current_x = target_x + (1.f - e.slide_t) * slide_off;

    ImVec2 min_pt(floorf(current_x), floorf(pos_y - box_h));
    ImVec2 max_pt(floorf(current_x + box_w), floorf(pos_y));

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    float a = e.alpha;

    ImU32 bg_t = a >= 1.f ? c_bg_t : col_alpha(c_bg_t, a);
    ImU32 bg_b = a >= 1.f ? c_bg_b : col_alpha(c_bg_b, a);
    dl->AddRectFilledMultiColor(min_pt, max_pt, bg_t, bg_t, bg_b, bg_b);

    ImVec2 strip_max(min_pt.x + strip_w, max_pt.y);
    ImU32 level_c = level_to_color(e.type);
    ImU32 level_d = col_lerp(level_c, c_outline, 0.4f);
    ImU32 strip_t = a >= 1.f ? level_c : col_alpha(level_c, a);
    ImU32 strip_b = a >= 1.f ? level_d : col_alpha(level_d, a);
    dl->AddRectFilledMultiColor(min_pt, strip_max, strip_t, strip_t, strip_b, strip_b);

    draw_combo_border(dl, min_pt, max_pt, a);

    float progress = e.exiting ? 0.f : (e.timer / theme::notify_duration);
    if (progress >= 0.f && progress <= 1.f) {
      float bar_y = max_pt.y - bar_h;
      ImVec2 bar_min(min_pt.x, bar_y);
      ImVec2 bar_max(max_pt.x, max_pt.y);
      dl->AddRectFilled(bar_min, bar_max, a >= 1.f ? c_inline : col_alpha(c_inline, a), 0.f);
      if (progress > 0.001f) {
        float fill_w = (max_pt.x - min_pt.x - 2.f) * progress;
        if (fill_w > 0.5f) {
          ImVec2 fill_max(min_pt.x + 1.f + fill_w, max_pt.y - 1.f);
          ImVec2 fill_min(min_pt.x + 1.f, bar_y + 1.f);
          ImU32 fill_t = a >= 1.f ? level_c : col_alpha(level_c, a);
          ImU32 fill_b = a >= 1.f ? level_d : col_alpha(level_d, a);
          dl->AddRectFilledMultiColor(fill_min, fill_max, fill_t, fill_t, fill_b, fill_b);
        }
      }
    }

    float txt_x = min_pt.x + strip_w + pad_x;
    float label_y = min_pt.y + pad_y;
    float msg_y = label_y + label_sz.y + label_gap;
    ImU32 label_col = a >= 1.f ? level_c : col_alpha(level_c, a);
    ImU32 txt_col = a >= 1.f ? c_txt : col_alpha(c_txt, a);
    dl->AddText(ImVec2(txt_x, label_y), label_col, label_str);
    dl->AddText(ImVec2(txt_x, msg_y), txt_col, str);

    pos_y -= box_h + spacing;
  }
}

}
