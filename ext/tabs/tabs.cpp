#include "tabs.h"
#include "../helpers/helper.h"
#include "imgui/imgui.h"

void RenderTabs(float x, float y, float width, const char** tab_names,
    int tab_count, int* selected_tab, float fade_alpha) {

  if (tab_count <= 0 || !selected_tab || *selected_tab < 0) {
    if (selected_tab)
      *selected_tab = 0;
  }
  if (selected_tab && *selected_tab >= tab_count)
    *selected_tab = tab_count - 1;

  ImDrawList* dl = ImGui::GetForegroundDrawList();
  float bar_h = theme::tab_bar_h;
  int n = (tab_count < 32) ? tab_count : 32;
  float gap = theme::tab_gap;
  float w_per_tab = (width - (float)(n - 1) * gap) / (float)n;

  float tab_x[32];
  float tab_w[32];
  for (int i = 0; i < n; i++) {
    tab_w[i] = w_per_tab;
    tab_x[i] = x + (float)i * (w_per_tab + gap);
  }

  static float anim_x_rel = 0.0f;
  static float anim_w = 0.0f;
  static bool did_init = false;

  int cur = *selected_tab;
  if (cur >= n)
    cur = n - 1;

  float target_x_rel = tab_x[cur] - x;
  float target_w = tab_w[cur];

  if (!did_init) {
    anim_x_rel = target_x_rel;
    anim_w = target_w;
    did_init = true;
  }

  float dt = ImGui::GetIO().DeltaTime;
  float lerp_factor = dt * theme::tab_speed;
  if (lerp_factor > 1.0f)
    lerp_factor = 1.0f;

  anim_x_rel += (target_x_rel - anim_x_rel) * lerp_factor;
  anim_w += (target_w - anim_w) * lerp_factor;

  float ind_x = x + anim_x_rel;
  int alpha_mask = (int)(255.0f * fade_alpha);

  ImU32 inactive_t = theme::col_alpha(theme::inactive_tab_t, alpha_mask);
  ImU32 inactive_b = theme::col_alpha(theme::inactive_tab_b, alpha_mask);
  ImU32 grad_t = theme::col_alpha(theme::topbar_grad_t, alpha_mask);
  ImU32 grad_b = theme::col_alpha(theme::topbar_grad_b, alpha_mask);
  ImU32 inline_c = theme::col_alpha(theme::topbar_inline, alpha_mask);
  ImU32 text_c = theme::col_alpha(theme::menu_text, alpha_mask);
  ImU32 sep1_c = theme::col_alpha(theme::tab_sep_1, alpha_mask);
  ImU32 brd_c = theme::col_alpha(theme::accent_border, alpha_mask);
  ImU32 ind_c = theme::col_alpha(theme::tab_selected_indicator, alpha_mask);

  dl->AddRectFilledMultiColor(
      ImVec2(x, y), ImVec2(x + width, y + bar_h),
      inactive_t, inactive_t, inactive_b, inactive_b);
  dl->AddRectFilledMultiColor(
      ImVec2(ind_x, y), ImVec2(ind_x + anim_w, y + bar_h),
      grad_t, grad_t, grad_b, grad_b);

  dl->AddRect(ImVec2(ind_x, y), ImVec2(ind_x + anim_w, y + bar_h),
      inline_c, 0.0f, 0, theme::out_thick);

  for (int s = 0; s < 2; s++) {
    float sx = (s == 0) ? ind_x : ind_x + anim_w - theme::out_thick;
    dl->AddRectFilled(ImVec2(sx, y), ImVec2(sx + theme::out_thick, y + bar_h), inline_c);
  }

  float ind_line_y = y + bar_h - 2.f;
  dl->AddRectFilled(ImVec2(ind_x + 1.f, ind_line_y), ImVec2(ind_x + anim_w - 1.f, ind_line_y + 2.f), ind_c);

  for (int i = 0; i < n; i++) {
    ImVec2 ts = ImGui::CalcTextSize(tab_names[i]);
    float txt_x = tab_x[i] + (tab_w[i] - ts.x) * 0.5f;
    float txt_y = y + (bar_h - ts.y) * 0.5f;
    dl->AddText(ImVec2(txt_x, txt_y), text_c, tab_names[i]);
  }

  float sep_y[] = { y + bar_h, y + bar_h + theme::out_thick };
  ImU32 sep_colors[] = { sep1_c, brd_c };
  for (int i = 0; i < theme::tab_sep_cnt; i++) {
    dl->AddRectFilled(ImVec2(x, sep_y[i]), ImVec2(x + width, sep_y[i] + theme::out_thick), sep_colors[i]);
  }

  ImVec2 mp = ImGui::GetIO().MousePos;
  static bool drag = false;
  if (ImGui::IsMouseReleased(0))
    drag = false;

  for (int i = 0; i < n; i++) {
    bool in_tab = mp.x >= tab_x[i] && mp.x < tab_x[i] + tab_w[i] && mp.y >= y && mp.y < y + bar_h;
    if (in_tab) {
      if (ImGui::IsMouseClicked(0))
        *selected_tab = i;
      if (ImGui::IsMouseDown(0) && i == cur)
        drag = true;
      break;
    }
  }

  if (drag && ImGui::IsMouseDown(0)) {
    for (int i = 0; i < n; i++) {
      bool in_tab = mp.x >= tab_x[i] && mp.x < tab_x[i] + tab_w[i] && mp.y >= y && mp.y < y + bar_h;
      if (in_tab) {
        *selected_tab = i;
        break;
      }
    }
  }
}
