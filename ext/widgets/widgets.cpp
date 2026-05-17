#define NOMINMAX
#include "widgets.h"
#include "../helpers/helper.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <windows.h>

namespace widgets {

static constexpr float hdr_h = 24.f;
static constexpr float item_h = 20.f;
static constexpr float pad_x = 10.f;
static constexpr float pad_y = 6.f;
static constexpr float spd = 12.f;

static constexpr ImU32 c_bg_t = IM_COL32(24, 21, 28, 255);
static constexpr ImU32 c_bg_b = IM_COL32(15, 13, 18, 255);
static constexpr ImU32 c_wbg = IM_COL32(30, 26, 36, 255);
static constexpr ImU32 c_hov = IM_COL32(45, 38, 56, 255);
static constexpr ImU32 c_txt = IM_COL32(184, 176, 192, 255);
static constexpr ImU32 c_txt_dim = IM_COL32(120, 112, 128, 255);
static constexpr ImU32 c_acc = IM_COL32(61, 53, 72, 255);
static constexpr ImU32 c_acc_d = IM_COL32(45, 38, 56, 255);
static constexpr ImU32 c_acc_mid = IM_COL32(53, 48, 69, 255);
static constexpr ImU32 c_outline = IM_COL32(15, 13, 18, 255);
static constexpr ImU32 c_inline = IM_COL32(37, 32, 48, 255);

static std::unordered_map<ImGuiID, float> s_anim;
static ImGuiID s_kb_listening = 0;
static bool s_kb_ignore_first = false;
static ImGuiID s_cur_section = 0;

struct section_state_t {
  bool collapsed = false;
  float anim_t = 1.f;
  float last_h = 400.f;
  bool fixed = false;
};

static std::unordered_map<ImGuiID, section_state_t> s_sections;

static float lerp_factor(float speed, float dt) {
  static int s_last_dt_key = -1;
  static float s_last_speed = -1.f;
  static float s_cached_factor = 0.f;
  int dt_key = (int)(dt * 1000.f);
  if (dt_key != s_last_dt_key || speed != s_last_speed) {
    s_last_dt_key = dt_key;
    s_last_speed = speed;
    s_cached_factor = 1.f - expf(-speed * dt);
  }
  return s_cached_factor;
}

static float tick(ImGuiID id, bool target, float speed = spd) {
  float& t = s_anim[id];
  float dt = ImGui::GetIO().DeltaTime;
  t += ((target ? 1.f : 0.f) - t) * lerp_factor(speed, dt);
  if (t < 0.1e-3f) t = 0.f;
  if (t > 0.999f) t = 1.f;
  return t;
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

static void draw_combo_border(ImDrawList* dl, ImVec2 min_pt, ImVec2 max_pt) {
  dl->AddRect(min_pt, max_pt, c_outline, 0.f, 0, 1.f);
  dl->AddRect(ImVec2(min_pt.x + 1.f, min_pt.y + 1.f), ImVec2(max_pt.x - 1.f, max_pt.y - 1.f), c_inline, 0.f, 0, 1.f);
}

static const char* vk_name(int vk) {
  switch (vk) {
  case VK_LSHIFT:   return "LSHIFT";
  case VK_RSHIFT:   return "RSHIFT";
  case VK_LCONTROL: return "LCTRL";
  case VK_RCONTROL: return "RCTRL";
  case VK_LMENU:    return "LALT";
  case VK_RMENU:    return "RALT";
  case VK_LBUTTON:  return "LMB";
  case VK_RBUTTON:  return "RMB";
  case VK_MBUTTON:  return "MMB";
  case VK_XBUTTON1: return "MB4";
  case VK_XBUTTON2: return "MB5";
  case VK_SPACE:    return "SPACE";
  case VK_RETURN:   return "ENTER";
  case VK_DELETE:   return "DEL";
  case VK_INSERT:   return "INS";
  case VK_HOME:     return "HOME";
  case VK_END:      return "END";
  case VK_PRIOR:    return "PGUP";
  case VK_NEXT:     return "PGDN";
  case VK_UP:       return "UP";
  case VK_DOWN:     return "DOWN";
  case VK_LEFT:     return "LEFT";
  case VK_RIGHT:    return "RIGHT";
  case 0:           return "NONE";
  default: {
    static char buf[16] = {};
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
      buf[0] = (char)vk;
      buf[1] = 0;
      return buf;
    }
    std::snprintf(buf, sizeof(buf), "%02X", vk);
    return buf;
  }
  }
}

bool checkbox(const char* label, bool& value) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float row_h = item_h + pad_y;
  float cb_sz = 14.f;
  ImGui::InvisibleButton(label, ImVec2(avail, row_h));
  if (ImGui::IsItemClicked())
    value = !value;
  ImGuiID id = ImGui::GetID(label) ^ 0xCB1;
  float t = tick(id, value);
  float th = tick(id ^ 0x11, ImGui::IsItemHovered());
  float cy = floorf(pos.y + (row_h - cb_sz) * 0.5f);
  ImVec2 bmin(pos.x, cy);
  ImVec2 bmax(pos.x + cb_sz, cy + cb_sz);
  dl->AddRectFilled(bmin, bmax, col_lerp(c_wbg, c_hov, th), 0);
  if (t > 0.01f) {
    ImU32 acc_a = col_alpha(c_acc, t);
    ImU32 acc_d_a = col_alpha(c_acc_d, t);
    dl->AddRectFilledMultiColor(bmin, bmax, acc_a, acc_a, acc_d_a, acc_d_a);
  }
  draw_combo_border(dl, bmin, bmax);
  float text_y = floorf(pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos.x + cb_sz + 8.f, text_y), c_txt, label);
  return ImGui::IsItemClicked();
}

bool checkbox_color(const char* label, bool& value, float color[4]) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float row_h = item_h + pad_y;
  float cb_sz = 14.f;
  float swatch_w = 20.f;
  ImGui::InvisibleButton(label, ImVec2(avail - swatch_w - 4.f, row_h));
  if (ImGui::IsItemClicked())
    value = !value;
  ImGuiID id = ImGui::GetID(label) ^ 0xCBC;
  float t = tick(id, value);
  float th = tick(id ^ 0x11, ImGui::IsItemHovered());
  float cy = floorf(pos.y + (row_h - cb_sz) * 0.5f);
  ImVec2 bmin(pos.x, cy);
  ImVec2 bmax(pos.x + cb_sz, cy + cb_sz);
  dl->AddRectFilled(bmin, bmax, col_lerp(c_wbg, c_hov, th), 0);
  if (t > 0.01f) {
    ImU32 acc_a = col_alpha(c_acc, t);
    ImU32 acc_d_a = col_alpha(c_acc_d, t);
    dl->AddRectFilledMultiColor(bmin, bmax, acc_a, acc_a, acc_d_a, acc_d_a);
  }
  draw_combo_border(dl, bmin, bmax);
  float text_y = floorf(pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos.x + cb_sz + 8.f, text_y), c_txt, label);
  float sw_y = floorf(pos.y + (row_h - 14.f) * 0.5f);
  ImVec2 sw_min(floorf(pos.x + avail - swatch_w), sw_y);
  char pop_id[128];
  std::snprintf(pop_id, sizeof(pop_id), "##pop_%s", label);
  if (ImGui::GetIO().MouseClicked[1] && ImGui::IsItemHovered())
    ImGui::OpenPopup(pop_id);
  ImGui::SetCursorScreenPos(sw_min);
  ImVec4 color_vec(color[0], color[1], color[2], color[3]);
  ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview;
  if (ImGui::ColorButton(pop_id, color_vec, picker_flags, ImVec2(swatch_w, 14.f)))
    ImGui::OpenPopup(pop_id);
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
  if (ImGui::BeginPopup(pop_id)) {
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    ImGui::ColorPicker4("##picker", color, flags);
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + row_h));
  ImGui::Dummy(ImVec2(0, 0));
  return ImGui::IsItemClicked();
}

bool color_edit(const char* label, float color[4]) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float row_h = item_h + pad_y;
  float swatch_w = 20.f;
  float text_y = floorf(pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos.x, text_y), c_txt, label);
  float sw_y = floorf(pos.y + (row_h - 14.f) * 0.5f);
  ImVec2 sw_min(floorf(pos.x + avail - swatch_w), sw_y);
  char pop_id[128];
  std::snprintf(pop_id, sizeof(pop_id), "##pop_ce_%s", label);
  ImGui::SetCursorScreenPos(sw_min);
  ImVec4 color_vec(color[0], color[1], color[2], color[3]);
  ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview;
  bool clicked = ImGui::ColorButton(pop_id, color_vec, picker_flags, ImVec2(swatch_w, 14.f));
  if (clicked)
    ImGui::OpenPopup(pop_id);
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
  if (ImGui::BeginPopup(pop_id)) {
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    ImGui::ColorPicker4("##picker", color, flags);
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + row_h));
  ImGui::Dummy(ImVec2(0, 0));
  return clicked;
}

bool checkbox_keybind(const char* label, bool& value, int& key) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float row_h = item_h + pad_y;
  float cb_sz = 14.f;
  float box_w = 60.f;
  ImGui::InvisibleButton(label, ImVec2(avail - box_w - 4.f, row_h));
  if (ImGui::IsItemClicked())
    value = !value;
  ImGuiID id = ImGui::GetID(label) ^ 0xCB2;
  float t = tick(id, value);
  float th = tick(id ^ 0x11, ImGui::IsItemHovered());
  float cy = floorf(pos.y + (row_h - cb_sz) * 0.5f);
  ImVec2 bmin(pos.x, cy);
  ImVec2 bmax(pos.x + cb_sz, cy + cb_sz);
  dl->AddRectFilled(bmin, bmax, col_lerp(c_wbg, c_hov, th), 0);
  if (t > 0.01f) {
    ImU32 acc_a = col_alpha(c_acc, t);
    ImU32 acc_d_a = col_alpha(c_acc_d, t);
    dl->AddRectFilledMultiColor(bmin, bmax, acc_a, acc_a, acc_d_a, acc_d_a);
  }
  draw_combo_border(dl, bmin, bmax);
  float text_y = floorf(pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos.x + cb_sz + 8.f, text_y), c_txt, label);
  float kb_x = floorf(pos.x + avail - box_w);
  float kb_y = floorf(pos.y + (row_h - item_h) * 0.5f);
  ImVec2 kb_min(kb_x, kb_y);
  ImVec2 kb_max(kb_min.x + box_w, kb_min.y + item_h);
  ImGuiID kb_id = ImGui::GetID(label) ^ 0x1B1;
  if (s_kb_listening == kb_id) {
    if (s_kb_ignore_first) {
      if (!ImGui::IsMouseDown(0))
        s_kb_ignore_first = false;
    } else {
      for (int vk = 1; vk < 256; vk++) {
        if (!(GetAsyncKeyState(vk) & 0x8000))
          continue;
        key = (vk == VK_ESCAPE) ? 0 : vk;
        s_kb_listening = 0;
        break;
      }
    }
  }
  ImGui::SetCursorScreenPos(kb_min);
  char kb_bid[128];
  std::snprintf(kb_bid, sizeof(kb_bid), "##kb_%s", label);
  if (ImGui::InvisibleButton(kb_bid, ImVec2(box_w, item_h))) {
    s_kb_listening = (s_kb_listening == kb_id) ? 0 : kb_id;
    s_kb_ignore_first = true;
  }
  float kb_th = tick(kb_id ^ 0xBB, ImGui::IsItemHovered() || (s_kb_listening == kb_id));
  dl->AddRectFilled(kb_min, kb_max, col_lerp(c_wbg, c_hov, kb_th), 0);
  draw_combo_border(dl, kb_min, kb_max);
  const char* kn = (s_kb_listening == kb_id) ? "..." : vk_name(key);
  ImVec2 kts = ImGui::CalcTextSize(kn);
  float kn_x = floorf(kb_min.x + (box_w - kts.x) * 0.5f);
  float kn_y = floorf(kb_min.y + (item_h - kts.y) * 0.5f);
  ImU32 kn_c = (s_kb_listening == kb_id) ? c_acc : c_txt;
  dl->AddText(ImVec2(kn_x, kn_y), kn_c, kn);
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + row_h));
  ImGui::Dummy(ImVec2(0, 0));
  return ImGui::IsItemClicked();
}

bool slider_scalar(const char* label, float& value, float v_min, float v_max, const char* fmt) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  const float lh = item_h;
  const float th = 16.f;
  const float gap = 4.f;
  const float total = lh + gap + th + pad_y;
  ImGui::InvisibleButton(label, ImVec2(avail, lh));
  float label_y = floorf(pos.y + (lh - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos.x, label_y), c_txt, label);
  bool changed = false;
  float track_y = floorf(pos.y + lh + gap);
  ImVec2 tr_min(pos.x, track_y);
  ImVec2 tr_max(pos.x + avail, track_y + th);
  ImGui::SetCursorScreenPos(tr_min);
  char tid[128];
  std::snprintf(tid, sizeof(tid), "##trk_%s", label);
  ImGui::InvisibleButton(tid, ImVec2(avail, th));
  if (ImGui::IsItemActive()) {
    float mouse_x = ImGui::GetIO().MousePos.x;
    float f = (mouse_x - tr_min.x) / (tr_max.x - tr_min.x);
    if (f < 0.f) f = 0.f;
    else if (f > 1.f) f = 1.f;
    float nv = v_min + f * (v_max - v_min);
    if (nv != value) {
      value = nv;
      changed = true;
    }
  }
  dl->AddRectFilled(tr_min, tr_max, c_wbg, 0);
  float frac = (value - v_min) / (v_max - v_min);
  float fill_x = floorf(tr_min.x + (tr_max.x - tr_min.x) * frac);
  if (fill_x > tr_min.x)
    dl->AddRectFilledMultiColor(tr_min, ImVec2(fill_x, tr_max.y), c_acc, c_acc, c_acc_d, c_acc_d);
  draw_combo_border(dl, tr_min, tr_max);
  char buf[64];
  std::snprintf(buf, sizeof(buf), fmt, value);
  ImVec2 bts = ImGui::CalcTextSize(buf);
  float buf_x = floorf(tr_min.x + (avail - bts.x) * 0.5f);
  float buf_y = floorf(tr_min.y + (th - bts.y) * 0.5f);
  dl->AddText(ImVec2(buf_x, buf_y), c_txt, buf);
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + total));
  ImGui::Dummy(ImVec2(0, 0));
  return changed;
}

bool slider_scalar(const char* label, int& value, int v_min, int v_max, const char* fmt) {
  float fv = (float)value;
  bool ch = slider_scalar(label, fv, (float)v_min, (float)v_max, fmt);
  value = (int)fv;
  return ch;
}

bool button(const char* label, bool active) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float bh = item_h + pad_y;
  ImGui::InvisibleButton(label, ImVec2(avail, bh));
  ImGuiID id = ImGui::GetID(label) ^ 0xB10;
  bool hot = ImGui::IsItemHovered() || ImGui::IsItemActive() || active;
  float th = tick(id, hot);
  ImU32 top_c = col_lerp(c_wbg, c_acc, th);
  ImU32 bot_c = col_lerp(c_wbg, c_acc_d, th);
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + avail, pos.y + bh), top_c, top_c, bot_c, bot_c);
  draw_combo_border(dl, pos, ImVec2(pos.x + avail, pos.y + bh));
  ImVec2 ts = ImGui::CalcTextSize(label);
  float txt_x = floorf(pos.x + (avail - ts.x) * 0.5f);
  float txt_y = floorf(pos.y + (bh - ts.y) * 0.5f);
  dl->AddText(ImVec2(txt_x, txt_y), c_txt, label);
  return ImGui::IsItemClicked();
}

bool multi_button(const char** labels, int count, int& selected) {
  if (count <= 0) return false;
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float bh = item_h + pad_y;
  float gap = 4.f;
  float bw = floorf((avail - gap * (count - 1)) / count);
  ImGuiID gid = ImGui::GetID("##mbg") ^ (ImGuiID)(uintptr_t)labels;
  float win_x = ImGui::GetWindowPos().x;
  float& ind_x_rel = s_anim[gid ^ 0x98];
  float tgt_x_rel = floorf((pos.x - win_x) + selected * (bw + gap));
  if (ind_x_rel < 0.1f)
    ind_x_rel = tgt_x_rel;
  float dt = ImGui::GetIO().DeltaTime;
  ind_x_rel += (tgt_x_rel - ind_x_rel) * lerp_factor(spd, dt);
  for (int i = 0; i < count; i++) {
    float bx = floorf(pos.x + i * (bw + gap));
    ImVec2 bmin(bx, pos.y);
    ImVec2 bmax(bx + bw, pos.y + bh);
    char bid[128];
    std::snprintf(bid, sizeof(bid), "%s##mb%d", labels[i], i);
    ImGui::SetCursorScreenPos(bmin);
    ImGui::InvisibleButton(bid, ImVec2(bw, bh));
    if (ImGui::IsItemClicked())
      selected = i;
    float hov_t = tick(ImGui::GetID(bid) ^ 0xB1B1, ImGui::IsItemHovered());
    dl->AddRectFilled(bmin, bmax, col_lerp(c_wbg, c_hov, hov_t), 0);
    draw_combo_border(dl, bmin, bmax);
    ImVec2 ts = ImGui::CalcTextSize(labels[i]);
    float t_x = floorf(bmin.x + (bw - ts.x) * 0.5f);
    float t_y = floorf(bmin.y + (bh - ts.y) * 0.5f);
    dl->AddText(ImVec2(t_x, t_y), c_txt, labels[i]);
  }
  float fx = floorf(win_x + ind_x_rel);
  dl->AddRectFilledMultiColor(ImVec2(fx, pos.y), ImVec2(fx + bw, pos.y + bh), c_acc, c_acc, c_acc_d, c_acc_d);
  dl->AddRect(ImVec2(fx, pos.y), ImVec2(fx + bw, pos.y + bh), c_outline, 0.f, 0, 1.f);
  dl->AddRect(ImVec2(fx + 1.f, pos.y + 1.f), ImVec2(fx + bw - 1.f, pos.y + bh - 1.f), c_acc_mid, 0.f, 0, 1.f);
  ImVec2 ts = ImGui::CalcTextSize(labels[selected]);
  float sel_tx = floorf(fx + (bw - ts.x) * 0.5f);
  float sel_ty = floorf(pos.y + (bh - ts.y) * 0.5f);
  dl->AddText(ImVec2(sel_tx, sel_ty), c_txt, labels[selected]);
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + bh + 4.f));
  ImGui::Dummy(ImVec2(0, 0));
  return ImGui::IsItemClicked();
}

bool begin_combo(const char* label, const char* preview, const char* const* items, int ic, int& cur_idx) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  float pos_x = ImGui::GetCursorScreenPos().x;
  float pos_y = ImGui::GetCursorScreenPos().y;
  float bh = item_h + pad_y;
  if (label && label[0] != '#') {
    float lbl_y = floorf(pos_y + (bh - ImGui::GetTextLineHeight()) * 0.5f);
    dl->AddText(ImVec2(pos_x, lbl_y), c_txt_dim, label);
    float lw = ImGui::CalcTextSize(label).x + 8.f;
    pos_x += lw;
    avail -= lw;
  }
  ImGui::SetCursorScreenPos(ImVec2(pos_x, pos_y));
  char bid[128];
  std::snprintf(bid, sizeof(bid), "##cmb_%s", label);
  ImGui::InvisibleButton(bid, ImVec2(avail, bh));
  ImGuiID id = ImGui::GetID(bid);
  bool& open = *(bool*)&s_anim[id ^ 0xFF01];
  if (ImGui::IsItemClicked())
    open = !open;
  float hov_t = tick(id, ImGui::IsItemHovered());
  dl->AddRectFilled(ImVec2(pos_x, pos_y), ImVec2(pos_x + avail, pos_y + bh), col_lerp(c_wbg, c_hov, hov_t), 0);
  draw_combo_border(dl, ImVec2(pos_x, pos_y), ImVec2(pos_x + avail, pos_y + bh));
  const char* preview_text = (cur_idx >= 0 && cur_idx < ic) ? items[cur_idx] : preview;
  float prev_y = floorf(pos_y + (bh - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos_x + 8.f, prev_y), c_txt, preview_text);
  float ax = floorf(pos_x + avail - 14.f);
  float ay = floorf(pos_y + bh * 0.5f);
  ImU32 arrow_c = col_lerp(c_txt_dim, c_acc, tick(id ^ 0xA1, open));
  dl->AddTriangleFilled(ImVec2(ax, ay - 3.f), ImVec2(ax + 7.f, ay - 3.f), ImVec2(ax + 3.5f, ay + 3.f), arrow_c);
  if (open) {
    float ih = item_h + 2.f;
    float pmin_y = pos_y + bh + 2.f;
    ImVec2 pmin(pos_x, pmin_y);
    ImVec2 pmax(pos_x + avail, pmin_y + ih * ic);
    ImDrawList* fg_dl = ImGui::GetForegroundDrawList();
    fg_dl->AddRectFilled(pmin, pmax, c_wbg, 0);
    draw_combo_border(fg_dl, pmin, pmax);
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    for (int i = 0; i < ic; i++) {
      float r_min_y = floorf(pmin.y + i * ih);
      float r_max_y = floorf(r_min_y + ih);
      ImVec2 r_min(pmin.x, r_min_y);
      ImVec2 r_max(pmax.x, r_max_y);
      bool hover = mouse_pos.x >= r_min.x && mouse_pos.x <= r_max.x && mouse_pos.y >= r_min.y && mouse_pos.y <= r_max.y;
      if (hover) {
        fg_dl->AddRectFilled(r_min, r_max, c_hov, 0);
        if (ImGui::IsMouseClicked(0)) {
          cur_idx = i;
          open = false;
        }
      }
      if (cur_idx == i)
        fg_dl->AddRectFilled(r_min, r_max, col_alpha(c_acc, 0.25f), 0);
      float item_y = floorf(r_min.y + (ih - ImGui::GetTextLineHeight()) * 0.5f);
      fg_dl->AddText(ImVec2(r_min.x + 8.f, item_y), c_txt, items[i]);
    }
    bool click_outside = ImGui::IsMouseClicked(0) && !ImGui::IsMouseHoveringRect(ImVec2(pos_x, pos_y), ImVec2(pos_x + avail, pos_y + bh));
    if (click_outside)
      open = false;
  }
  ImGui::SetCursorScreenPos(ImVec2(pos_x, pos_y + bh + 4.f));
  ImGui::Dummy(ImVec2(0, 0));
  return ImGui::IsItemClicked();
}

bool keybind(const char* label, int& key) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::GetContentRegionAvail().x;
  ImVec2 pos = ImGui::GetCursorScreenPos();
  float bh = item_h + pad_y;
  float box_w = 60.f;
  ImGuiID id = ImGui::GetID(label);
  if (s_kb_listening == id) {
    if (s_kb_ignore_first) {
      if (!ImGui::IsMouseDown(0))
        s_kb_ignore_first = false;
    } else {
      for (int vk = 1; vk < 256; vk++) {
        if (!(GetAsyncKeyState(vk) & 0x8000))
          continue;
        key = (vk == VK_ESCAPE) ? 0 : vk;
        s_kb_listening = 0;
        break;
      }
    }
  }
  float bmin_x = floorf(pos.x + avail - box_w);
  float bmin_y = floorf(pos.y + (bh - item_h) * 0.5f);
  ImVec2 bmin(bmin_x, bmin_y);
  ImVec2 bmax(bmin.x + box_w, bmin.y + item_h);
  ImGui::SetCursorScreenPos(bmin);
  if (ImGui::InvisibleButton(label, ImVec2(box_w, item_h))) {
    s_kb_listening = (s_kb_listening == id) ? 0 : id;
    s_kb_ignore_first = true;
  }
  float th = tick(id ^ 0xBB, ImGui::IsItemHovered() || (s_kb_listening == id));
  dl->AddRectFilled(bmin, bmax, col_lerp(c_wbg, c_hov, th), 0);
  draw_combo_border(dl, bmin, bmax);
  const char* kn = (s_kb_listening == id) ? "..." : vk_name(key);
  ImVec2 kts = ImGui::CalcTextSize(kn);
  float kn_x = floorf(bmin.x + (box_w - kts.x) * 0.5f);
  float kn_y = floorf(bmin.y + (item_h - kts.y) * 0.5f);
  ImU32 kn_c = (s_kb_listening == id) ? c_acc : c_txt;
  dl->AddText(ImVec2(kn_x, kn_y), kn_c, kn);
  float lbl_y = floorf(pos.y + (bh - ImGui::GetTextLineHeight()) * 0.5f);
  dl->AddText(ImVec2(pos.x, lbl_y), c_txt, label);
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + bh + 4.f));
  ImGui::Dummy(ImVec2(0, 0));
  return ImGui::IsItemClicked();
}

const char* get_key_name(int vk) {
  return vk_name(vk);
}

void keybind_list(float x, float y, float width, const char** labels,
    const int* keys, int count, float alpha) {
  if (!labels || !keys || count <= 0) return;
  const float out = theme::out_thick;
  const float tb_h = theme::topbar_h;
  const float row_h = item_h + pad_y;
  const float inner_pad = pad_x;
  float content_h = (float)count * row_h + pad_y * 2.f;
  float total_h = tb_h + out + content_h;
  int alpha_v = (int)(255.f * alpha);
  if (alpha_v <= 0) return;

  ImDrawList* dl = ImGui::GetForegroundDrawList();
  ImVec2 min_pt(floorf(x), floorf(y));
  ImVec2 max_pt(floorf(x + width), floorf(y + total_h));

  ImU32 topbar_t = theme::col_alpha(theme::topbar_grad_t, alpha_v);
  ImU32 topbar_b = theme::col_alpha(theme::topbar_grad_b, alpha_v);
  dl->AddRectFilledMultiColor(min_pt, ImVec2(max_pt.x, min_pt.y + tb_h), topbar_t, topbar_t, topbar_b, topbar_b);
  dl->AddRect(min_pt, ImVec2(max_pt.x, min_pt.y + tb_h), theme::col_alpha(theme::topbar_inline, alpha_v), 0.f, 0, out);
  dl->AddLine(ImVec2(min_pt.x, min_pt.y + tb_h), ImVec2(max_pt.x, min_pt.y + tb_h), theme::col_alpha(theme::accent_border, alpha_v), out);

  const char* title = "Keybinds";
  ImVec2 title_sz = ImGui::CalcTextSize(title);
  float title_y = min_pt.y + (tb_h - title_sz.y) * 0.5f;
  dl->AddText(ImVec2(min_pt.x + inner_pad, title_y), theme::col_alpha(theme::menu_text, alpha_v), title);

  float cont_y = min_pt.y + tb_h + out;
  ImU32 c_top = IM_COL32(24, 21, 28, alpha_v);
  ImU32 c_mid = IM_COL32(22, 20, 26, alpha_v);
  ImU32 c_btm = IM_COL32(18, 16, 22, alpha_v);
  float cont_h = content_h;
  dl->AddRectFilledMultiColor(ImVec2(min_pt.x, cont_y), ImVec2(max_pt.x, cont_y + cont_h * 0.5f), c_top, c_top, c_mid, c_mid);
  dl->AddRectFilledMultiColor(ImVec2(min_pt.x, cont_y + cont_h * 0.5f), ImVec2(max_pt.x, max_pt.y), c_mid, c_mid, c_btm, c_btm);

  float row_y = cont_y + pad_y;
  for (int i = 0; i < count; i++) {
    const char* key_str = get_key_name(keys[i]);
    ImVec2 label_sz = ImGui::CalcTextSize(labels[i]);
    ImVec2 key_sz = ImGui::CalcTextSize(key_str);
    float line_h = (label_sz.y > key_sz.y ? label_sz.y : key_sz.y);
    float text_y = row_y + (row_h - line_h) * 0.5f;
    dl->AddText(ImVec2(min_pt.x + inner_pad, text_y), theme::col_alpha(theme::menu_text, alpha_v), labels[i]);
    ImU32 key_c = (alpha >= 1.f) ? c_txt_dim : col_alpha(c_txt_dim, alpha);
    dl->AddText(ImVec2(max_pt.x - inner_pad - key_sz.x, text_y), key_c, key_str);
    row_y += row_h;
  }

  dl->AddRect(min_pt, max_pt, theme::col_alpha(theme::accent_border, alpha_v), 0.f, 0, out);
  ImU32 in_c = (alpha >= 1.f) ? c_inline : col_alpha(c_inline, alpha);
  dl->AddRect(ImVec2(min_pt.x + 1.f, min_pt.y + 1.f), ImVec2(max_pt.x - 1.f, max_pt.y - 1.f), in_c, 0.f, 0, out);
}

bool section_begin(const char* label, const char** tl, int tc, int* ts, float height) {
  ImDrawList* dl = ImGui::GetWindowDrawList();
  float avail = ImGui::CalcItemWidth();
  float py = ImGui::GetCursorPosY();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImGuiID id = ImGui::GetID(label);
  section_state_t& st = s_sections[id];
  s_cur_section = id;
  ImGui::PushID(label);
  if (height > 0.f) {
    st.last_h = floorf(height - hdr_h - 4.f);
    st.fixed = true;
  }
  float dt = ImGui::GetIO().DeltaTime;
  float anim_delta = (st.collapsed ? 0.f : 1.f) - st.anim_t;
  st.anim_t += anim_delta * lerp_factor(theme::tab_speed, dt);
  if (st.anim_t < 0.f) st.anim_t = 0.f;
  if (st.anim_t > 1.f) st.anim_t = 1.f;
  ImVec2 hmin(floorf(pos.x), floorf(pos.y));
  ImVec2 hmax(floorf(pos.x + avail), floorf(pos.y + hdr_h));
  if (tl && tc > 0 && ts) {
    float tw = avail / tc;
    float& tind_x_rel = s_anim[id ^ 0x99];
    float tgt = (*ts) * tw;
    if (tind_x_rel < 0.1f) tind_x_rel = tgt;
    tind_x_rel += (tgt - tind_x_rel) * lerp_factor(theme::tab_speed, dt);
    ImU32 inact_t = theme::col_alpha(theme::inactive_tab_t, 255);
    ImU32 inact_b = theme::col_alpha(theme::inactive_tab_b, 255);
    dl->AddRectFilledMultiColor(hmin, hmax, inact_t, inact_t, inact_b, inact_b);
    ImVec2 tmin(floorf(pos.x + tind_x_rel), hmin.y);
    ImVec2 tmax(floorf(pos.x + tind_x_rel + tw), hmax.y);
    if (tmax.x > hmax.x) tmax.x = hmax.x;
    ImU32 grad_t = theme::col_alpha(theme::topbar_grad_t, 255);
    ImU32 grad_b = theme::col_alpha(theme::topbar_grad_b, 255);
    dl->AddRectFilledMultiColor(ImVec2(tmin.x + 1.f, tmin.y + 1.f), ImVec2(tmax.x - 1.f, tmax.y - 1.f), grad_t, grad_t, grad_b, grad_b);
    dl->AddRect(ImVec2(tmin.x + 1.f, tmin.y + 1.f), ImVec2(tmax.x - 1.f, tmax.y - 1.f), c_inline, 0.f, 0, 1.f);
    draw_combo_border(dl, hmin, hmax);
    for (int i = 0; i < tc; i++) {
      float tx = floorf(pos.x + i * tw);
      ImVec2 rm(tx, hmin.y);
      ImVec2 rmx(floorf(pos.x + (i + 1) * tw), hmax.y);
      if (ImGui::GetIO().MouseClicked[0] && ImGui::IsMouseHoveringRect(rm, rmx))
        *ts = i;
      ImVec2 tsz = ImGui::CalcTextSize(tl[i]);
      float tab_tx = floorf(tx + (tw - tsz.x) * 0.5f);
      float tab_ty = floorf(hmin.y + (hdr_h - tsz.y) * 0.5f);
      ImU32 tab_c = (*ts == i) ? c_txt : c_txt_dim;
      dl->AddText(ImVec2(tab_tx, tab_ty), tab_c, tl[i]);
    }
  } else {
    ImU32 grad_t = theme::col_alpha(theme::topbar_grad_t, 255);
    ImU32 grad_b = theme::col_alpha(theme::topbar_grad_b, 255);
    dl->AddRectFilledMultiColor(hmin, hmax, grad_t, grad_t, grad_b, grad_b);
    draw_combo_border(dl, hmin, hmax);
    float hdr_tx = floorf(hmin.x + pad_x);
    float hdr_ty = floorf(hmin.y + (hdr_h - ImGui::GetTextLineHeight()) * 0.5f);
    dl->AddText(ImVec2(hdr_tx, hdr_ty), c_txt, label);
    float cmin_x = floorf(hmax.x - 20.f);
    float cmin_y = floorf(hmin.y + (hdr_h - 16.f) * 0.5f);
    ImVec2 cmin(cmin_x, cmin_y);
    ImVec2 cmax(cmin.x + 16.f, cmin.y + 16.f);
    ImGui::SetCursorScreenPos(cmin);
    if (ImGui::InvisibleButton("##clps", ImVec2(16.f, 16.f)))
      st.collapsed = !st.collapsed;
    bool hover_collapse = ImGui::IsMouseHoveringRect(cmin, cmax);
    float collapse_t = tick(id ^ 0xC01, hover_collapse);
    dl->AddRectFilled(cmin, cmax, col_lerp(c_wbg, c_hov, collapse_t), 0);
    draw_combo_border(dl, cmin, cmax);
    const char* sy = st.collapsed ? "+" : "-";
    ImVec2 mts = ImGui::CalcTextSize(sy);
    float sy_x = floorf(cmin.x + (16.f - mts.x) * 0.5f);
    float sy_y = floorf(cmin.y + (16.f - mts.y) * 0.5f);
    dl->AddText(ImVec2(sy_x, sy_y), c_txt, sy);
  }
  float ch = st.last_h * st.anim_t;
  ImGui::SetCursorPosY(py + hdr_h + 4.f);
  if (ch < 1.f) {
    ImGui::Dummy(ImVec2(avail, 0));
    ImGui::PopID();
    return false;
  }
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad_x, pad_y));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(c_outline));
  ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  bool vis = ImGui::BeginChild(id, ImVec2(avail, ch), true, child_flags);
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
  if (vis) {
    ImVec2 cwp = ImGui::GetWindowPos();
    ImVec2 cws = ImGui::GetWindowSize();
    ImDrawList* wdl = ImGui::GetWindowDrawList();
    wdl->AddRectFilledMultiColor(cwp, ImVec2(cwp.x + cws.x, cwp.y + cws.y), c_bg_t, c_bg_t, c_bg_b, c_bg_b);
    wdl->AddRect(ImVec2(cwp.x + 1.f, cwp.y + 1.f), ImVec2(cwp.x + cws.x - 1.f, cwp.y + cws.y - 1.f), c_inline, 0.f, 0, 1.f);
  }
  return vis;
}

void section_end() {
  if (!s_cur_section) return;
  section_state_t& st = s_sections[s_cur_section];
  if (!st.collapsed && st.anim_t > 0.99f && !st.fixed) {
    float h = ImGui::GetCursorPosY();
    if (h > 4.f)
      st.last_h = h + pad_y;
  }
  ImGui::EndChild();
  ImGui::PopID();
}

}
