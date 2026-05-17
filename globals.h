#pragma once
#include <cstdint>

inline bool box_esp = false;
inline int box_esp_type = 0;
inline bool box_fill = false;
inline bool box_fill_gradient = false;
inline bool box_fill_gradient_rotate = false;
inline int box_fill_type = 0;
inline float box_esp_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline float box_fill_top[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
inline float box_fill_bottom[4] = { 0.0f, 0.0f, 0.0f, 0.5f };
inline bool skeleton = false;
inline float skeleton_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool healthbar = false;
inline float healthbar_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
inline bool health_text = false;
inline float health_text_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool name = false;
inline float name_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool distance = false;
inline float distance_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool rig_type = false;
inline float rig_type_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool tool_esp = false;
inline float tool_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool chinahat = false;
inline float chinahat_color[4] = { 1.0f, 0.8f, 0.2f, 0.42f };
inline bool chams = false;
inline float chams_color[4] = { 0.396f, 0.420f, 0.722f, 0.5f };
inline float esp_render_distance = 500.0f;
inline bool union_chams = false;
inline bool outline_chams = false;
inline float outline_chams_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool mesh_chams = false;
inline float mesh_chams_color[4] = { 0.396f, 0.420f, 0.722f, 0.5f };
inline bool memory_mesh_chams = false;
inline float memory_mesh_chams_color[4] = { 0.145f, 0.850f, 0.580f, 0.42f };
inline bool memory_union_chams = false;
inline bool memory_outline_chams = false;
inline float memory_outline_chams_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline bool render_local_player = false;
inline bool render_expanded_hitbox = false;
inline bool aimbot_enabled = false;
inline int aimbot_keybind = 0;
inline bool aimbot_keybind_waiting = false;
inline int aimbot_aim_type = 0;
inline bool sticky_aim = false;
inline int aimbot_part = 0;
inline bool show_fov = false;
inline float fov_size = 100.0f;
inline float smoothing_x = 5.0f;
inline float smoothing_y = 5.0f;
inline bool prediction_enabled = false;
inline float prediction_x = 5.0f;
inline float prediction_y = 5.0f;

inline bool vsync = false;
inline bool show_fps = false;
inline bool show_watermark = true;
inline bool esp_preview_3d = true;
inline float menu_alpha = 0.0f;

inline float fps = 0.0f;

inline bool walkspeed_enabled = false;
inline int walkspeed_keybind = 0;
inline bool walkspeed_keybind_waiting = false;
inline float walkspeed_value = 50.0f;

inline bool flight_enabled = false;
inline int flight_keybind = 0;
inline bool flight_keybind_waiting = false;
inline float flight_value = 50.0f;

inline bool hitbox_expander_enabled = false;
inline float hitbox_expander_value = 10.0f;
inline bool aimviewer = false;

inline bool skybox_changer_enabled = false;
inline int skybox_type = 0;
inline char skybox_debug_msg[384] = "";

inline bool noclip_enabled = false;
inline int noclip_keybind = 0;

inline bool korblox_enabled = false;
inline uint64_t current_place_id = 0;
inline bool dex_explorer = false;

inline bool rivals_skin_changer_enabled = false;
inline bool blade_ball_auto_parry = false;
inline bool blade_ball_ball_esp = false;
inline float blade_ball_parry_distance = 20.0f;
inline float blade_ball_parry_height = 12.0f;
