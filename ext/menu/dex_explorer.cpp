#include "dex_explorer.h"
#include "../helpers/helper.h"
#include "imgui/imgui.h"
#include "globals.h"
#include "overlay.h"
#include "offsets.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stack>
#include <string>
#include <chrono>
#include <algorithm>
#include <functional>
#include <cstring>

extern uintptr_t g_base_address;

bool is_valid_address(uintptr_t address);

template <typename T>
T read(uint64_t address);

std::string fetchstring(uint64_t address);

struct dm_instance {
    uint64_t address;

    bool is_valid() const {
        return address != 0 && is_valid_address(address);
    }

    std::string get_name() const {
        return fetchstring(read<uint64_t>(address + Offsets::Instance::Name));
    }

    std::string get_class_name() const {
        uint64_t class_desc = read<uint64_t>(address + Offsets::Instance::ClassDescriptor);
        return fetchstring(read<uint64_t>(class_desc + Offsets::Instance::ClassName));
    }

    dm_instance read_parent() const {
        uint64_t parent_addr = read<uint64_t>(address + Offsets::Instance::Parent);
        return dm_instance{ parent_addr };
    }

    std::vector<dm_instance> get_children() const {
        std::vector<dm_instance> children;
        uint64_t begin = read<uint64_t>(address + Offsets::Instance::ChildrenStart);
        uint64_t end = read<uint64_t>(begin + Offsets::Instance::ChildrenEnd);
        if (!begin) return children;
        if (!end) return children;
        for (auto inst = read<uint64_t>(begin); inst != end; inst += 16u) {
            uint64_t child_addr = read<uint64_t>(inst);
            children.emplace_back(dm_instance{ child_addr });
        }
        return children;
    }

    dm_instance find_first_child(const std::string& name) const {
        for (auto child : get_children()) {
            if (child.get_name() == name) return child;
        }
        return dm_instance{};
    }

    dm_instance read_service(const std::string& class_name) const {
        for (auto child : get_children()) {
            if (child.get_class_name() == class_name) return child;
        }
        return dm_instance{};
    }
};

static dm_instance get_datamodel() {
    uint64_t fake_data_model_ptr = read<uint64_t>(g_base_address + Offsets::FakeDataModel::Pointer);
    if (!is_valid_address(fake_data_model_ptr)) return dm_instance{};
    uint64_t real_data_model = read<uint64_t>(fake_data_model_ptr + Offsets::FakeDataModel::RealDataModel);
    if (!is_valid_address(real_data_model)) return dm_instance{};
    return dm_instance{ real_data_model };
}

static float dex_ease(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

void RenderDexExplorer() {
    if (!dex_explorer) return;

    static dm_instance selected_node;
    static std::unordered_map<uint64_t, std::vector<dm_instance>> node_children_cache;
    static std::unordered_map<uint64_t, std::string> node_name_cache;
    static std::unordered_map<uint64_t, std::string> node_class_cache;
    static std::unordered_map<uint64_t, std::string> node_path_cache;
    static std::vector<dm_instance> search_results;
    static bool show_search_results = false;
    static char search_query[128] = "";
    static auto last_cache_refresh = std::chrono::steady_clock::now();

    auto cache_node = [&](dm_instance& node) {
        if (!node.is_valid()) return;
        if (node_children_cache.find(node.address) == node_children_cache.end()) {
            node_children_cache[node.address] = node.get_children();
            node_name_cache[node.address] = node.get_name();
            node_class_cache[node.address] = node.get_class_name();
            std::string path = node_name_cache[node.address];
            dm_instance parent = node.read_parent();
            while (parent.is_valid()) {
                if (node_children_cache.find(parent.address) == node_children_cache.end()) {
                    node_children_cache[parent.address] = parent.get_children();
                    node_name_cache[parent.address] = parent.get_name();
                    node_class_cache[parent.address] = parent.get_class_name();
                }
                std::string parent_name = node_name_cache[parent.address];
                if (!parent_name.empty()) {
                    path = parent_name + "." + path;
                }
                parent = parent.read_parent();
            }
            node_path_cache[node.address] = path;
        }
    };

    dm_instance datamodel = get_datamodel();
    if (!datamodel.is_valid()) return;

    dm_instance root_instance(datamodel.address);

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cache_refresh).count() >= 2) {
        node_children_cache.clear();
        node_name_cache.clear();
        node_class_cache.clear();
        node_path_cache.clear();
        search_results.clear();
        show_search_results = false;
        last_cache_refresh = now;
    }

    if (node_children_cache.find(root_instance.address) == node_children_cache.end()) {
        cache_node(root_instance);
    }

    ImGuiIO& io = ImGui::GetIO();
    static ImVec2 wnd_size(720.0f, 620.0f);
    static ImVec2 wnd_pos(io.DisplaySize.x - wnd_size.x - 40.0f, 80.0f);
    static bool dragging = false;
    static float drag_x = 0.0f;
    static float drag_y = 0.0f;
    static double open_time = -1.0;

    double now_time = ImGui::GetTime();
    if (open_time < 0.0) open_time = now_time;

    float t = (float)(now_time - open_time) / theme::menu_expand_dur;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    float alpha = dex_ease(t);

    wnd_pos.x = io.DisplaySize.x - wnd_size.x - 40.0f + drag_x;
    wnd_pos.y = 80.0f + drag_y;

    ImGui::SetNextWindowPos(wnd_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(wnd_size, ImGuiCond_Always);

    ImVec4 bg_col(theme::menu_bg.x / 255.f, theme::menu_bg.y / 255.f, theme::menu_bg.z / 255.f, alpha);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    if (!overlay::visible) {
        flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    if (ImGui::Begin("DexExplorerWindow", nullptr, flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float tb_h = theme::topbar_h;
        float out = theme::out_thick;
        int alpha_v = (int)(255.f * alpha);

        if (ImGui::IsMouseReleased(0)) dragging = false;
        ImVec2 m_pos = ImGui::GetIO().MousePos;
        bool in_title = m_pos.x >= wp.x && m_pos.x <= wp.x + ws.x && m_pos.y >= wp.y && m_pos.y < wp.y + tb_h;
        if (in_title && ImGui::IsMouseClicked(0)) dragging = true;
        if (dragging && ImGui::IsMouseDown(0)) {
            drag_x += ImGui::GetIO().MouseDelta.x;
            drag_y += ImGui::GetIO().MouseDelta.y;
        }

        ImU32 topbar_t = theme::col_alpha(theme::topbar_grad_t, alpha_v);
        ImU32 topbar_b = theme::col_alpha(theme::topbar_grad_b, alpha_v);
        dl->AddRectFilledMultiColor(wp, ImVec2(wp.x + ws.x, wp.y + tb_h), topbar_t, topbar_t, topbar_b, topbar_b);
        dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + tb_h), theme::col_alpha(theme::topbar_inline, alpha_v), 0.f, 0, out);
        dl->AddLine(ImVec2(wp.x, wp.y + tb_h), ImVec2(wp.x + ws.x, wp.y + tb_h), theme::col_alpha(theme::accent_border, alpha_v), out);

        float title_y = wp.y + (tb_h - ImGui::CalcTextSize("Dex Explorer").y) * 0.5f;
        dl->AddText(ImVec2(wp.x + 8.f, title_y), theme::col_alpha(theme::menu_text, alpha_v), "Dex Explorer");

        float content_y = tb_h + out;
        ImVec2 cont_pos(wp.x + 8.f, wp.y + content_y + 4.f);
        ImVec2 cont_sz(ws.x - 16.f, ws.y - content_y - 12.f);

        ImU32 c_top = IM_COL32(24, 21, 28, alpha_v);
        ImU32 c_mid = IM_COL32(22, 20, 26, alpha_v);
        ImU32 c_btm = IM_COL32(18, 16, 22, alpha_v);
        dl->AddRectFilledMultiColor(cont_pos, ImVec2(cont_pos.x + cont_sz.x, cont_pos.y + cont_sz.y * 0.5f), c_top, c_top, c_mid, c_mid);
        dl->AddRectFilledMultiColor(ImVec2(cont_pos.x, cont_pos.y + cont_sz.y * 0.5f), ImVec2(cont_pos.x + cont_sz.x, cont_pos.y + cont_sz.y), c_mid, c_mid, c_btm, c_btm);

        ImGui::SetCursorPos(ImVec2(8.f, content_y + 6.f));
        ImGui::BeginChild("DexContent", ImVec2(ws.x - 16.f, ws.y - content_y - 10.f), false, ImGuiWindowFlags_NoScrollbar);

        ImGui::SetCursorPos(ImVec2(6.f, 6.f));
        ImGui::PushItemWidth(cont_sz.x * 0.55f);
        bool search_changed = ImGui::InputTextWithHint("##DexSearch", "Search...", search_query, IM_ARRAYSIZE(search_query), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();

        if (search_changed) {
            search_results.clear();
            show_search_results = std::strlen(search_query) > 0;
            if (show_search_results && std::strlen(search_query) >= 1) {
                std::string query = search_query;
                std::transform(query.begin(), query.end(), query.begin(), ::tolower);
                std::function<void(dm_instance&)> search_instance = [&](dm_instance& inst) {
                    if (!inst.is_valid()) return;
                    if (search_results.size() >= 100) return;
                    cache_node(inst);
                    std::string name = node_name_cache[inst.address];
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (name.find(query) != std::string::npos) {
                        search_results.push_back(inst);
                    }
                    for (auto& child : inst.get_children()) {
                        search_instance(child);
                    }
                };
                dm_instance workspace = root_instance.find_first_child("Workspace");
                if (workspace.is_valid()) {
                    search_instance(workspace);
                }
                dm_instance players = root_instance.read_service("Players");
                if (players.is_valid()) {
                    for (auto& player : players.get_children()) {
                        search_instance(player);
                    }
                }
                dm_instance teams = root_instance.find_first_child("Teams");
                if (teams.is_valid()) {
                    search_instance(teams);
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(80.f, 0.f))) {
            node_children_cache.clear();
            node_name_cache.clear();
            node_class_cache.clear();
            node_path_cache.clear();
            search_results.clear();
            show_search_results = false;
            std::memset(search_query, 0, sizeof(search_query));
        }

        float area_y = 36.f;
        float area_h = cont_sz.y - area_y - 8.f;
        float left_w = cont_sz.x * 0.55f;
        float right_w = cont_sz.x - left_w - 8.f;

        ImGui::SetCursorPos(ImVec2(6.f, area_y));
        ImGui::BeginChild("DexTree", ImVec2(left_w, area_h), true);
        ImGui::Text("Explorer");
        ImGui::Separator();

        if (show_search_results && std::strlen(search_query) > 0) {
            ImGui::Text("Search Results (%d):", (int)search_results.size());
            ImGui::Separator();
            if (!search_results.empty()) {
                for (auto& node : search_results) {
                    if (!node.is_valid()) continue;
                    cache_node(node);
                    std::string display_text = node_name_cache[node.address];
                    std::string class_name = node_class_cache[node.address];
                    std::string full_text = display_text + " [" + class_name + "]";
                    bool is_selected = (selected_node.address == node.address);
                    if (ImGui::Selectable(full_text.c_str(), is_selected)) {
                        selected_node = node;
                    }
                }
            } else {
                ImGui::Text("No results found");
            }
        } else {
            if (node_children_cache.find(root_instance.address) != node_children_cache.end()) {
                for (auto& child : node_children_cache[root_instance.address]) {
                    std::stack<std::pair<dm_instance, int>> stack;
                    stack.push({ child, 0 });
                    while (!stack.empty()) {
                        auto pair = stack.top();
                        stack.pop();
                        dm_instance node = pair.first;
                        int indent_level = pair.second;
                        if (!node.is_valid()) continue;
                        cache_node(node);
                        ImGui::SetCursorPosX(20.0f * indent_level);
                        ImGui::PushID((int)node.address);
                        const std::vector<dm_instance>& children = node_children_cache[node.address];
                        bool has_children = !children.empty();
                        std::string class_name = node_class_cache[node.address];
                        std::string display_text = node_name_cache[node.address] + " [" + class_name + "]";
                        ImGuiTreeNodeFlags node_flags = has_children ? 0 : ImGuiTreeNodeFlags_Leaf;
                        node_flags |= ImGuiTreeNodeFlags_OpenOnArrow;
                        if (selected_node.address == node.address) node_flags |= ImGuiTreeNodeFlags_Selected;
                        bool expanded = ImGui::TreeNodeEx(display_text.c_str(), node_flags);
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            selected_node = node;
                        }
                        if (expanded) {
                            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                                stack.push({ *it, indent_level + 1 });
                            }
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                }
            }
        }

        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("DexProperties", ImVec2(right_w, area_h), true);
        ImGui::Text("Properties");
        ImGui::Separator();

        if (selected_node.is_valid()) {
            cache_node(selected_node);
            float col1_width = 90.0f;
            ImGui::Text("Path:");
            ImGui::SameLine(col1_width);
            ImGui::TextWrapped("%s", node_path_cache[selected_node.address].c_str());
            ImGui::Text("Name:");
            ImGui::SameLine(col1_width);
            ImGui::Text("%s", node_name_cache[selected_node.address].c_str());
            ImGui::Text("Class:");
            ImGui::SameLine(col1_width);
            ImGui::Text("%s", node_class_cache[selected_node.address].c_str());
            dm_instance parent = selected_node.read_parent();
            std::string parent_name = parent.is_valid() ? parent.get_name() : "None";
            ImGui::Text("Parent:");
            ImGui::SameLine(col1_width);
            ImGui::Text("%s", parent_name.c_str());
            ImGui::Text("Address:");
            ImGui::SameLine(col1_width);
            ImGui::Text("0x%llX", (unsigned long long)selected_node.address);
        } else {
            ImGui::Text("No object selected");
        }

        ImGui::EndChild();

        ImDrawList* fg = ImGui::GetForegroundDrawList();
        fg->AddRect(ImVec2(wp.x - out, wp.y - out), ImVec2(wp.x + ws.x + out, wp.y + ws.y + out), theme::col_alpha(theme::accent_border, alpha_v), 0.f, 0, out);

        ImGui::PopStyleVar();
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

