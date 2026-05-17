#include "menu.h"
#include "../helpers/helper.h"
#include "../helpers/notif.h"
#include "../tabs/tabs.h"
#include "imgui/imgui.h"
#include "globals.h"
#include "features/skybox_changer/skybox_changer.h"

static float ease(float t) {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  return t * t * (3.0f - 2.0f * t);
}

static const ImGuiWindowFlags menu_flags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

static const float keybind_list_w = 180.f;
static const float keybind_list_gap = 8.f;

void RenderMenu() {
  static double expand_start = -1.0;
  static float drag_x = 0.0f;
  static float drag_y = 0.0f;
  static bool dragging = false;
  static const char* tab_names[] = { "Combat", "ESP", "Exploits", "Settings" };
  static int selected = 0;

  const double now = ImGui::GetTime();
  if (expand_start < 0.0)
    expand_start = now;

  float expand_t = (float)(now - expand_start) / theme::menu_expand_dur;
  if (expand_t > 1.0f) expand_t = 1.0f;
  if (expand_t < 0.0f) expand_t = 0.0f;

  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  const float menu_w = theme::menu_size.x;
  const float menu_h = theme::menu_size.y * ease(expand_t);
  const float cx = disp.x * 0.5f + drag_x;
  const float cy = disp.y * 0.5f + drag_y;

  float menu_x = cx - menu_w * 0.5f;
  float menu_y = cy - menu_h * 0.5f;
  float list_x = menu_x - keybind_list_w - keybind_list_gap;
  ImGui::SetNextWindowPos(ImVec2(menu_x, menu_y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(menu_w, menu_h), ImGuiCond_Always);

  float alpha = 0.0f;
  if (expand_t >= 1.0f) {
    float fade_t = (float)(now - expand_start - theme::menu_expand_dur) / theme::menu_fade_dur;
    alpha = ease(fade_t);
  }
  menu_alpha = alpha;

  ImVec4 bg_col(theme::menu_bg.x / 255.f, theme::menu_bg.y / 255.f, theme::menu_bg.z / 255.f, alpha);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);

  if (ImGui::Begin("Menu", nullptr, menu_flags)) {
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    const ImVec2 wp = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float out = theme::out_thick;
    const float tb_h = theme::topbar_h;
    const int alpha_v = (int)(255.f * alpha);

    if (ImGui::IsMouseReleased(0))
      dragging = false;
    const ImVec2 m_pos = ImGui::GetIO().MousePos;
    bool in_title = m_pos.x >= wp.x && m_pos.x <= wp.x + ws.x && m_pos.y >= wp.y && m_pos.y < wp.y + tb_h;
    if (in_title && ImGui::IsMouseClicked(0))
      dragging = true;
    if (dragging && ImGui::IsMouseDown(0)) {
      drag_x += ImGui::GetIO().MouseDelta.x;
      drag_y += ImGui::GetIO().MouseDelta.y;
    }

    ImU32 topbar_t = theme::col_alpha(theme::topbar_grad_t, alpha_v);
    ImU32 topbar_b = theme::col_alpha(theme::topbar_grad_b, alpha_v);
    dl->AddRectFilledMultiColor(wp, ImVec2(wp.x + ws.x, wp.y + tb_h), topbar_t, topbar_t, topbar_b, topbar_b);
    dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + tb_h), theme::col_alpha(theme::topbar_inline, alpha_v), 0.f, 0, out);
    dl->AddLine(ImVec2(wp.x, wp.y + tb_h), ImVec2(wp.x + ws.x, wp.y + tb_h), theme::col_alpha(theme::accent_border, alpha_v), out);

    float menu_title_y = wp.y + (tb_h - ImGui::CalcTextSize("popstar").y) * 0.5f;
    dl->AddText(ImVec2(wp.x + 8.f, menu_title_y), theme::col_alpha(theme::menu_text, alpha_v), "popstar");

    RenderTabs(wp.x, wp.y + tb_h + out, ws.x, tab_names, 4, &selected, alpha);

    float cont_y_off = tb_h + theme::tab_bar_h + (float)theme::tab_sep_cnt * out;
    ImVec2 cont_pos(wp.x, wp.y + cont_y_off);
    ImVec2 cont_sz(ws.x, ws.y - cont_y_off);

    ImU32 c_top = IM_COL32(24, 21, 28, alpha_v);
    ImU32 c_mid = IM_COL32(22, 20, 26, alpha_v);
    ImU32 c_btm = IM_COL32(18, 16, 22, alpha_v);
    dl->AddRectFilledMultiColor(cont_pos, ImVec2(cont_pos.x + cont_sz.x, cont_pos.y + cont_sz.y * 0.5f), c_top, c_top, c_mid, c_mid);
    dl->AddRectFilledMultiColor(ImVec2(cont_pos.x, cont_pos.y + cont_sz.y * 0.5f), ImVec2(cont_pos.x + cont_sz.x, cont_pos.y + cont_sz.y), c_mid, c_mid, c_btm, c_btm);

    ImGui::SetCursorPos(ImVec2(0.f, cont_y_off));
    if (ImGui::BeginChild("TabContent", cont_sz, false, ImGuiWindowFlags_NoScrollbar)) {
      const float start_y = 8.f;
      const float btm_pad = 10.f;
      const float col_w = (ws.x - 24.f) * 0.5f;
      const float avail_h = (ws.y - cont_y_off) - start_y - btm_pad;

      auto render_col = [&](int col_idx) {
        ImGui::SetCursorPos(ImVec2(8.f + col_idx * (col_w + 8.f), start_y));
        if (alpha < 0.01f) {
          ImGui::Dummy(ImVec2(col_w, avail_h));
          return;
        }
        ImGui::BeginGroup();
        ImGui::PushItemWidth(col_w);

        if (selected == 0) {
          if (col_idx == 0) {
            static int sub = 0;
            static const char* subs[] = { "General", "Accuracy" };
            if (widgets::section_begin("Aimbot", subs, 2, &sub, avail_h)) {
              if (sub == 0) {
                widgets::checkbox_keybind("Enable Aimbot", aimbot_enabled, aimbot_keybind);
                widgets::checkbox("Show FOV", show_fov);
                widgets::slider_scalar("FOV Size", fov_size, 10.0f, 500.0f, "%.0f");
              } else {
                widgets::slider_scalar("Smoothing X", smoothing_x, 1.0f, 30.0f, "%.1f");
                widgets::slider_scalar("Smoothing Y", smoothing_y, 1.0f, 30.0f, "%.1f");
                widgets::checkbox("Prediction", prediction_enabled);
                if (prediction_enabled) {
                  widgets::slider_scalar("Prediction X", prediction_x, 1.0f, 30.0f, "%.1f");
                  widgets::slider_scalar("Prediction Y", prediction_y, 1.0f, 30.0f, "%.1f");
                }
              }
              widgets::section_end();
            }
          } else {
            if (widgets::section_begin("Targeting", nullptr, 0, nullptr, avail_h)) {
              static const char* aim_type_names[] = { "Cam Aim", "Mouse Aim", "Silent" };
              widgets::begin_combo("Aim Type", aim_type_names[aimbot_aim_type], aim_type_names, 3, aimbot_aim_type);
              widgets::checkbox("Sticky Aim", sticky_aim);
              static const char* aim_parts[] = { "Head", "Upper Torso", "Lower Torso", "Left Hand", "Right Hand", "Left Foot", "Right Foot" };
              widgets::begin_combo("Aim Part", aim_parts[aimbot_part], aim_parts, 7, aimbot_part);
              widgets::section_end();
            }
          }
        } else if (selected == 1) {
          if (col_idx == 0) {
            if (widgets::section_begin("Player ESP", nullptr, 0, nullptr, avail_h)) {
              widgets::checkbox_color("Box ESP", box_esp, box_esp_color);
              widgets::checkbox_color("Skeleton ESP", skeleton, skeleton_color);
              widgets::checkbox_color("China Hat", chinahat, chinahat_color);
              widgets::checkbox_color("Health Bar", healthbar, healthbar_color);
              widgets::checkbox_color("Health Text", health_text, health_text_color);
              widgets::checkbox_color("Name", name, name_color);
              widgets::checkbox_color("Distance", distance, distance_color);
              widgets::checkbox_color("Rig Type", rig_type, rig_type_color);
              widgets::checkbox_color("Tool", tool_esp, tool_color);
              //widgets::checkbox_color("Mesh Chams", mesh_chams, mesh_chams_color);
              widgets::checkbox_color("Memory Mesh Chams", memory_mesh_chams, memory_mesh_chams_color);
              if (!memory_mesh_chams) {
                memory_union_chams = false;
                memory_outline_chams = false;
              }
              if (memory_mesh_chams) {
                widgets::checkbox("Union MemChams", memory_union_chams);
                widgets::checkbox_color("Outline MemChams", memory_outline_chams, memory_outline_chams_color);
              }
              if (!mesh_chams) {
                union_chams = false;
                outline_chams = false;
              }
              if (mesh_chams) {
                widgets::checkbox("Union Chams", union_chams);
                if (union_chams) {
                  widgets::checkbox_color("Outline Chams", outline_chams, outline_chams_color);
                }
              }
              widgets::checkbox("Render Local Player", render_local_player);
              widgets::checkbox("Render Expanded Hitbox", render_expanded_hitbox);
              widgets::section_end();
            }
          } else {
            if (widgets::section_begin("Options", nullptr, 0, nullptr, avail_h)) {
              static const char* box_types[] = { "Bounding", "Corner" };
              widgets::begin_combo("Box Style", box_types[box_esp_type], box_types, 2, box_esp_type);
              widgets::checkbox("Box Fill", box_fill);
              if (box_fill) {
                widgets::checkbox("Fill Gradient", box_fill_gradient);
                if (box_fill_gradient) {
                  widgets::checkbox("Fill Rotation", box_fill_gradient_rotate);
                  if (box_fill_gradient_rotate) {
                    static const char* fill_types[] = { "Side", "Bottom", "Spin" };
                    widgets::begin_combo("Rotation Type", fill_types[box_fill_type], fill_types, 3, box_fill_type);
                  }
                  widgets::color_edit("Fill Top", box_fill_top);
                  widgets::color_edit("Fill Bottom", box_fill_bottom);
                } else {
                  widgets::color_edit("Fill Color", box_fill_top);
                }
              }
              widgets::slider_scalar("Render Distance", esp_render_distance, 0.0f, 500.0f, "%.0f");
              widgets::section_end();
            }
          }
        } else if (selected == 2) {
          if (col_idx == 0) {
            if (widgets::section_begin("Movement", nullptr, 0, nullptr, avail_h)) {
              widgets::checkbox_keybind("WalkSpeed", walkspeed_enabled, walkspeed_keybind);
              if (walkspeed_enabled) {
                widgets::slider_scalar("Speed Value", walkspeed_value, 0.0f, 1000.0f, "%.0f");
              }
              widgets::checkbox_keybind("Flight", flight_enabled, flight_keybind);
              if (flight_enabled) {
                widgets::slider_scalar("Flight Speed", flight_value, 0.0f, 500.0f, "%.0f");
              }
              widgets::checkbox_keybind("Noclip", noclip_enabled, noclip_keybind);
              widgets::section_end();
            }
          } else {
            static int exploits_sub = 0;
            static const char* exploits_subs[] = { "Misc", "Model" };
            if (widgets::section_begin("Misc", exploits_subs, 2, &exploits_sub, avail_h)) {
              if (exploits_sub == 0) {
                widgets::checkbox("Blade Ball Auto Parry", blade_ball_auto_parry);
                if (blade_ball_auto_parry) {
                  widgets::checkbox("Ball ESP", blade_ball_ball_esp);
                  widgets::slider_scalar("Parry Distance", blade_ball_parry_distance, 5.0f, 80.0f, "%.0f");
                  widgets::slider_scalar("Parry Height", blade_ball_parry_height, 2.0f, 40.0f, "%.0f");
                } else {
                  blade_ball_ball_esp = false;
                }
                widgets::checkbox("Hitbox Expander", hitbox_expander_enabled);
                if (hitbox_expander_enabled) {
                  widgets::slider_scalar("Hitbox Size", hitbox_expander_value, 1.0f, 500.0f, "%.1f");
                }
                widgets::checkbox("Aimviewer", aimviewer);
                widgets::checkbox("Skybox Changer", skybox_changer_enabled);
                if (skybox_changer_enabled) {
                  widgets::begin_combo("Skybox Type", features::k_skybox_names[skybox_type], features::k_skybox_names, features::k_skybox_count, skybox_type);
                  if (skybox_debug_msg[0]) {
                    ImGui::TextUnformatted(skybox_debug_msg);
                  }
                }
              } else {
                widgets::checkbox("Korblox", korblox_enabled);
                //widgets::checkbox("Rivals Skin Changer (beta dont use its useless rn)", rivals_skin_changer_enabled);
              }
              widgets::section_end();
            }
          }
        } else if (selected == 3) {
          if (col_idx == 0) {
            if (widgets::section_begin("Display", nullptr, 0, nullptr, avail_h)) {
              widgets::checkbox("VSync", vsync);
              widgets::checkbox("Show FPS", show_fps);
              widgets::checkbox("Dex Explorer", dex_explorer);
              widgets::checkbox("3d esp preview", esp_preview_3d);
              widgets::section_end();
            }
          } else {
            if (widgets::section_begin("Watermark", nullptr, 0, nullptr, avail_h)) {
              widgets::checkbox("Show Watermark", show_watermark);
              if (widgets::button("Test notification"))
                notify::print(notify::notify_info, "Notification test");
              if (widgets::button("Test success"))
                notify::print(notify::notify_success, "Saved successfully");
              widgets::section_end();
            }
          }
        }
        ImGui::PopItemWidth();
        ImGui::EndGroup();
      };

      render_col(0);
      ImGui::SameLine(0, 0);
      render_col(1);
    }
    ImGui::EndChild();

    ImDrawList* fg_dl = ImGui::GetForegroundDrawList();
    fg_dl->AddRect(ImVec2(wp.x - out, wp.y - out), ImVec2(wp.x + ws.x + out, wp.y + ws.y + out),
        theme::col_alpha(theme::accent_border, alpha_v), 0.f, 0, out);

    ImGui::PopStyleVar();
  }
  ImGui::End();

  if (alpha > 0.f) {
    const char* kbl_labels[] = { "Aimbot Key", "WalkSpeed Key", "Flight Key" };
    const int kbl_keys[] = { aimbot_keybind, walkspeed_keybind, flight_keybind };
    widgets::keybind_list(list_x, menu_y, keybind_list_w, kbl_labels, kbl_keys, 3, alpha);
  }

  ImGui::PopStyleVar(5);
  ImGui::PopStyleColor();
}
