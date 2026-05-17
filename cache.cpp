    #include "cache.h"
#include "game.h"
#include "memory.h"
#include "offsets.h"
#include "globals.h"
#include <vector>
#include <cstring>
#include <cmath>
#include <unordered_map>

namespace cache {
    static std::vector<uintptr_t> s_players_with_rig;
    static std::vector<EspEntity> s_esp_entities;
    static std::vector<SkeletonEntity> s_skeleton_entities;
    static LocalPlayerData s_local_player = { 0.0f, 0.0f, 0.0f, false, 0, 0 };
    static DWORD s_last_update_tick = 0;
    static constexpr DWORD k_update_interval_ms = 1;
    static bool s_is_phantom_forces = false;

    static bool match_part_name(const std::string& name, bool pf) {
        if (pf) {
            return name == "Head" ||
                name == "UpperTorso" ||
                name == "pfLimbs1" ||
                name == "pfLimbs2" ||
                name == "pfLimbs3" ||
                name == "pfLimbs4" ||
                name == "pfLimbs5";
        }
        return name == "Head" ||
            name == "UpperTorso" ||
            name == "LowerTorso" ||
            name == "Torso" ||
            name == "LeftHand" ||
            name == "RightHand" ||
            name == "LeftFoot" ||
            name == "RightFoot" ||
            name == "Left Arm" ||
            name == "Right Arm" ||
            name == "Left Leg" ||
            name == "Right Leg" ||
            name == "LeftUpperArm" ||
            name == "RightUpperArm" ||
            name == "LeftLowerArm" ||
            name == "RightLowerArm" ||
            name == "LeftUpperLeg" ||
            name == "RightUpperLeg" ||
            name == "LeftLowerLeg" ||
            name == "RightLowerLeg";
    }

    static void collect_parts(const instance& character, uintptr_t* primitives, uintptr_t* part_addresses, char (*part_names)[32], size_t& primitive_count, bool& has_humanoid, float& health, float& max_health, float& root_x, float& root_y, float& root_z) {
        primitive_count = 0;
        has_humanoid = false;
        health = 0.0f;
        max_health = 100.0f;
        root_x = root_y = root_z = 0.0f;
        if (!character.is_valid()) return;
        bool got_root = false;
        for (const instance& child : character.get_children()) {
            if (!child.is_valid()) continue;
            const std::string name = child.get_name();
            if (name == "Humanoid") {
                has_humanoid = true;
                health = read<float>(child.address + Offsets::Humanoid::Health);
                max_health = read<float>(child.address + Offsets::Humanoid::MaxHealth);
                if (max_health <= 0.0f) max_health = 100.0f;
                continue;
            }
            if (name == "HumanoidRootPart" && !got_root) {
                uintptr_t prim = read<uintptr_t>(child.address + Offsets::BasePart::Primitive);
                if (is_valid_address(prim)) {
                    root_x = read<float>(prim + Offsets::Primitive::Position);
                    root_y = read<float>(prim + Offsets::Primitive::Position + 4);
                    root_z = read<float>(prim + Offsets::Primitive::Position + 8);
                    got_root = true;
                }
            }
            if (!match_part_name(name, s_is_phantom_forces)) continue;
            if (primitive_count >= k_esp_primitive_capacity) continue;
            uintptr_t primitive = read<uintptr_t>(child.address + Offsets::BasePart::Primitive);
            if (!is_valid_address(primitive)) continue;
            primitives[primitive_count] = primitive;
            part_addresses[primitive_count] = child.address;
            size_t copy_len = name.size() < 31 ? name.size() : 31;
            memcpy(part_names[primitive_count], name.c_str(), copy_len);
            part_names[primitive_count][copy_len] = '\0';
            primitive_count += 1;
        }
    }

    static void collect_skeleton(const instance& character, SkeletonEntity& skel, bool& has_humanoid) {
        memset(&skel, 0, sizeof(skel));
        has_humanoid = false;
        skel.is_r15 = false;
        skel.is_phantom_forces = false;
        if (!character.is_valid()) return;
        for (const instance& child : character.get_children()) {
            if (!child.is_valid()) continue;
            const std::string name = child.get_name();
            if (name == "Humanoid") {
                has_humanoid = true;
                continue;
            }
            uintptr_t prim = read<uintptr_t>(child.address + Offsets::BasePart::Primitive);
            if (!is_valid_address(prim)) continue;
            if (name == "Head") skel.head = prim;
            else if (name == "UpperTorso") { skel.upper_torso = prim; skel.is_r15 = true; }
            else if (name == "LowerTorso") skel.lower_torso = prim;
            else if (name == "Torso") skel.upper_torso = prim;
            else if (name == "LeftUpperArm") skel.left_upper_arm = prim;
            else if (name == "RightUpperArm") skel.right_upper_arm = prim;
            else if (name == "LeftLowerArm") skel.left_lower_arm = prim;
            else if (name == "RightLowerArm") skel.right_lower_arm = prim;
            else if (name == "LeftHand") skel.left_hand = prim;
            else if (name == "RightHand") skel.right_hand = prim;
            else if (name == "LeftUpperLeg") skel.left_upper_leg = prim;
            else if (name == "RightUpperLeg") skel.right_upper_leg = prim;
            else if (name == "LeftLowerLeg") skel.left_lower_leg = prim;
            else if (name == "RightLowerLeg") skel.right_lower_leg = prim;
            else if (name == "LeftFoot") skel.left_foot = prim;
            else if (name == "RightFoot") skel.right_foot = prim;
            else if (name == "Left Arm") skel.left_hand = prim;
            else if (name == "Right Arm") skel.right_hand = prim;
            else if (name == "Left Leg") skel.left_foot = prim;
            else if (name == "Right Leg") skel.right_foot = prim;
            else if (name == "pfLimbs1") skel.pf_limbs[0] = prim;
            else if (name == "pfLimbs2") skel.pf_limbs[1] = prim;
            else if (name == "pfLimbs3") skel.pf_limbs[2] = prim;
            else if (name == "pfLimbs4") skel.pf_limbs[3] = prim;
            else if (name == "pfLimbs5") skel.pf_limbs[4] = prim;
        }
        if (s_is_phantom_forces && skel.head && skel.upper_torso) skel.is_phantom_forces = true;
    }

    static void update_special_place(const instance& dm) {
        s_players_with_rig.clear();
        s_esp_entities.clear();
        s_skeleton_entities.clear();
        instance workspace = dm.read_service("Workspace");
        if (!workspace.is_valid()) return;
        instance players_folder;
        for (const instance& child : workspace.get_children()) {
            if (!child.is_valid()) continue;
            if (child.get_name() == "Players") {
                players_folder = child;
                break;
            }
        }
        if (!players_folder.is_valid()) return;
        for (const instance& team_folder : players_folder.get_children()) {
            if (!team_folder.is_valid()) continue;
            for (const instance& model : team_folder.get_children()) {
                if (!model.is_valid()) continue;
                if (model.get_class_name() != "Model") continue;
                uintptr_t torso_prim = 0;
                uintptr_t head_prim = 0;
                struct PartInfo {
                    uintptr_t prim;
                    uintptr_t part_addr;
                    float x;
                    float y;
                    float z;
                };
                std::vector<PartInfo> parts;
                parts.reserve(k_esp_primitive_capacity);
                float torso_x = 0.0f;
                float torso_y = 0.0f;
                float torso_z = 0.0f;
                for (const instance& part : model.get_children()) {
                    if (!part.is_valid()) continue;
                    std::string cls = part.get_class_name();
                    if (cls != "Part" && cls != "MeshPart") continue;
                    uintptr_t prim = read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
                    if (!is_valid_address(prim)) continue;
                    float px = read<float>(prim + Offsets::Primitive::Position);
                    float py = read<float>(prim + Offsets::Primitive::Position + 4);
                    float pz = read<float>(prim + Offsets::Primitive::Position + 8);
                    bool has_spot = false;
                    bool has_billboard = false;
                    for (const instance& child : part.get_children()) {
                        if (!child.is_valid()) continue;
                        std::string ccls = child.get_class_name();
                        if (ccls == "SpotLight") has_spot = true;
                        else if (ccls == "BillboardGui") has_billboard = true;
                    }
                    if (has_spot && !torso_prim) {
                        torso_prim = prim;
                        torso_x = px;
                        torso_y = py;
                        torso_z = pz;
                    }
                    if (has_billboard && !head_prim) {
                        head_prim = prim;
                    }
                    if (parts.size() < k_esp_primitive_capacity) {
                        PartInfo info;
                        info.prim = prim;
                        info.part_addr = part.address;
                        info.x = px;
                        info.y = py;
                        info.z = pz;
                        parts.push_back(info);
                    }
                }
                if (!torso_prim || !head_prim) continue;
                SkeletonEntity skel{};
                skel.player_address = 0;
                skel.head = head_prim;
                skel.upper_torso = torso_prim;
                skel.lower_torso = 0;
                skel.is_r15 = false;
                skel.is_phantom_forces = true;
                for (int i = 0; i < 5; ++i) {
                    skel.pf_limbs[i] = 0;
                }
                int limb_index = 0;
                for (const PartInfo& p : parts) {
                    if (p.prim == torso_prim || p.prim == head_prim) continue;
                    if (limb_index < 5) {
                        skel.pf_limbs[limb_index] = p.prim;
                        limb_index += 1;
                    }
                }
                s_skeleton_entities.push_back(skel);
                EspEntity entity{};
                entity.player_address = 0;
                entity.character_address = model.address;
                entity.user_id = 0;
                entity.is_r15 = false;
                entity.primitive_count = 0;
                size_t idx = 0;
                for (const PartInfo& p : parts) {
                    if (idx >= k_esp_primitive_capacity) break;
                    entity.primitives[idx] = p.prim;
                    entity.part_addresses[idx] = p.part_addr;
                    if (p.prim == head_prim) {
                        const char* n = "Head";
                        memcpy(entity.part_names[idx], n, 5);
                        entity.part_names[idx][5] = '\0';
                    } else if (p.prim == torso_prim) {
                        const char* n = "UpperTorso";
                        memcpy(entity.part_names[idx], n, 10);
                        entity.part_names[idx][10] = '\0';
                    } else {
                        int pf_index = -1;
                        for (int i = 0; i < 5; ++i) {
                            if (skel.pf_limbs[i] == p.prim) {
                                pf_index = i;
                                break;
                            }
                        }
                        if (pf_index >= 0 && pf_index < 5) {
                            char buf[16] = {};
                            if (pf_index == 0) memcpy(buf, "pfLimbs1", 8);
                            else if (pf_index == 1) memcpy(buf, "pfLimbs2", 8);
                            else if (pf_index == 2) memcpy(buf, "pfLimbs3", 8);
                            else if (pf_index == 3) memcpy(buf, "pfLimbs4", 8);
                            else if (pf_index == 4) memcpy(buf, "pfLimbs5", 8);
                            memcpy(entity.part_names[idx], buf, 8);
                            entity.part_names[idx][8] = '\0';
                        } else {
                            entity.part_names[idx][0] = '\0';
                        }
                    }
                    idx += 1;
                }
                entity.primitive_count = idx;
                memset(entity.name, 0, sizeof(entity.name));
                memset(entity.display_name, 0, sizeof(entity.display_name));
                memset(entity.tool_name, 0, sizeof(entity.tool_name));
                entity.health = 0.0f;
                entity.max_health = 100.0f;
                entity.root_x = torso_x;
                entity.root_y = torso_y;
                entity.root_z = torso_z;
                s_esp_entities.push_back(entity);
            }
        }
    }

    static void update_bad_business(const instance& dm) {
        s_players_with_rig.clear();
        s_esp_entities.clear();
        s_skeleton_entities.clear();
        instance workspace = dm.read_service("Workspace");
        if (!workspace.is_valid()) return;
        instance characters_folder;
        for (const instance& child : workspace.get_children()) {
            if (!child.is_valid()) continue;
            if (child.get_name() == "Characters") {
                characters_folder = child;
                break;
            }
        }
        if (!characters_folder.is_valid()) return;
        for (const instance& model : characters_folder.get_children()) {
            if (!model.is_valid()) continue;
            if (model.get_class_name() != "Model") continue;
            instance body;
            for (const instance& child : model.get_children()) {
                if (!child.is_valid()) continue;
                if (child.get_name() == "Body") {
                    body = child;
                    break;
                }
            }
            if (!body.is_valid()) continue;
            SkeletonEntity skel{};
            skel.player_address = 0;
            skel.is_r15 = false;
            skel.is_phantom_forces = false;
            uintptr_t root_prim = 0;
            float root_x = 0.0f;
            float root_y = 0.0f;
            float root_z = 0.0f;
            EspEntity entity{};
            entity.player_address = 0;
            entity.character_address = model.address;
            entity.user_id = 0;
            entity.is_r15 = false;
            entity.primitive_count = 0;
            memset(entity.name, 0, sizeof(entity.name));
            memset(entity.display_name, 0, sizeof(entity.display_name));
            memset(entity.tool_name, 0, sizeof(entity.tool_name));
            entity.health = 0.0f;
            entity.max_health = 100.0f;
            size_t idx = 0;
            for (const instance& part : body.get_children()) {
                if (!part.is_valid()) continue;
                std::string name = part.get_name();
                uintptr_t prim = read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
                if (!is_valid_address(prim)) continue;
                if (name == "Head") {
                    skel.head = prim;
                } else if (name == "Chest") {
                    skel.upper_torso = prim;
                    if (!root_prim) root_prim = prim;
                } else if (name == "Abdomen") {
                    skel.lower_torso = prim;
                    if (!root_prim) root_prim = prim;
                } else if (name == "LeftArm") {
                    skel.left_hand = prim;
                } else if (name == "RightArm") {
                    skel.right_hand = prim;
                } else if (name == "LeftLeg") {
                    skel.left_foot = prim;
                } else if (name == "RightLeg") {
                    skel.right_foot = prim;
                }
                if (idx < k_esp_primitive_capacity) {
                    entity.primitives[idx] = prim;
                    entity.part_addresses[idx] = part.address;
                    size_t copy_len = name.size() < 31 ? name.size() : 31;
                    memcpy(entity.part_names[idx], name.c_str(), copy_len);
                    entity.part_names[idx][copy_len] = '\0';
                    idx += 1;
                }
            }
            if (root_prim) {
                root_x = read<float>(root_prim + Offsets::Primitive::Position);
                root_y = read<float>(root_prim + Offsets::Primitive::Position + 4);
                root_z = read<float>(root_prim + Offsets::Primitive::Position + 8);
            }
            if (!skel.head || !skel.upper_torso) continue;
            entity.primitive_count = idx;
            entity.root_x = root_x;
            entity.root_y = root_y;
            entity.root_z = root_z;
            s_skeleton_entities.push_back(skel);
            s_esp_entities.push_back(entity);
        }
    }

    void Update(uintptr_t base) {
        DWORD now = GetTickCount();
        if (s_last_update_tick != 0 && (now - s_last_update_tick) < k_update_interval_ms) return;
        s_last_update_tick = now;

        s_is_phantom_forces = (game::GetGameName(base) == "[SPRING UPDATE!] Phantom Forces") || (current_place_id == 292439477);

        instance dm = game::ReadDatamodel(base);
        if (!dm.is_valid()) return;
        if (current_place_id == 292439477) {
            update_special_place(dm);
            return;
        }
        if (current_place_id == 3233893879) {
            update_bad_business(dm);
            return;
        }
        s_players_with_rig.clear();
        s_esp_entities.clear();
        s_skeleton_entities.clear();
        s_local_player.valid = false;
        s_local_player.humanoid_address = 0;
        s_local_player.hrp_primitive = 0;
        instance players_service = dm.read_service("Players");
        if (!players_service.is_valid()) return;
        instance local = players_service.local_player();

        if (local.is_valid()) {
            instance local_char = local.model_instance();
            if (local_char.is_valid()) {
                bool got_hrp = false;
                bool got_humanoid = false;
                for (const instance& ch : local_char.get_children()) {
                    if (!ch.is_valid()) continue;
                    std::string ch_name = ch.get_name();
                    if (ch_name == "HumanoidRootPart" && !got_hrp) {
                        uintptr_t prim = read<uintptr_t>(ch.address + Offsets::BasePart::Primitive);
                        if (is_valid_address(prim)) {
                            s_local_player.x = read<float>(prim + Offsets::Primitive::Position);
                            s_local_player.y = read<float>(prim + Offsets::Primitive::Position + 4);
                            s_local_player.z = read<float>(prim + Offsets::Primitive::Position + 8);
                            s_local_player.hrp_primitive = prim;
                            s_local_player.valid = true;
                        }
                        got_hrp = true;
                    }
                    if (ch_name == "Humanoid" && !got_humanoid) {
                        s_local_player.humanoid_address = ch.address;
                        got_humanoid = true;
                    }
                    if (got_hrp && got_humanoid) break;
                }
            }
        }

        for (instance child : players_service.get_children()) {
            if (!child.is_valid()) continue;
            if (local.is_valid() && child.address == local.address) continue;
            if (child.get_class_name() != "Player") continue;
            instance character = child.model_instance();
            uintptr_t primitives[k_esp_primitive_capacity] = {};
            uintptr_t part_addresses[k_esp_primitive_capacity] = {};
            char part_names[k_esp_primitive_capacity][32] = {};
            size_t primitive_count = 0;
            bool has_humanoid = false;
            float health = 0.0f, max_health = 100.0f;
            float rx = 0.0f, ry = 0.0f, rz = 0.0f;
            collect_parts(character, primitives, part_addresses, part_names, primitive_count, has_humanoid, health, max_health, rx, ry, rz);
            if (!has_humanoid) continue;
            s_players_with_rig.push_back(child.address);

            bool skel_humanoid = false;
            SkeletonEntity skel{};
            collect_skeleton(character, skel, skel_humanoid);
            if (skel.head && skel.upper_torso) {
                skel.player_address = child.address;
                s_skeleton_entities.push_back(skel);
            }
            if (primitive_count > 0) {
                EspEntity entity{};
                entity.player_address = child.address;
                entity.character_address = character.address;
                entity.user_id = read<int64_t>(child.address + Offsets::Player::UserId);
                entity.is_r15 = skel.is_r15;
                entity.primitive_count = primitive_count;
                for (size_t i = 0; i < primitive_count; ++i) {
                    entity.primitives[i] = primitives[i];
                    entity.part_addresses[i] = part_addresses[i];
                    memcpy(entity.part_names[i], part_names[i], 32);
                }
                std::string pname = child.get_name();
                memset(entity.name, 0, sizeof(entity.name));
                size_t copy_len = pname.size() < 63 ? pname.size() : 63;
                memcpy(entity.name, pname.c_str(), copy_len);
                memset(entity.display_name, 0, sizeof(entity.display_name));
                uintptr_t dn_ptr = read<uintptr_t>(child.address + Offsets::Player::DisplayName);
                if (is_valid_address(dn_ptr)) {
                    std::string dname = fetchstring(dn_ptr);
                    copy_len = dname.size() < 63 ? dname.size() : 63;
                    memcpy(entity.display_name, dname.c_str(), copy_len);
                }
                memset(entity.tool_name, 0, sizeof(entity.tool_name));
                for (const instance& ch : character.get_children()) {
                    if (!ch.is_valid()) continue;
                    if (ch.get_class_name() == "Tool") {
                        std::string tname = ch.get_name();
                        copy_len = tname.size() < 63 ? tname.size() : 63;
                        memcpy(entity.tool_name, tname.c_str(), copy_len);
                        break;
                    }
                }
                entity.health = health;
                entity.max_health = max_health;
                entity.root_x = rx;
                entity.root_y = ry;
                entity.root_z = rz;
                s_esp_entities.push_back(entity);
            }
        }

        if (render_local_player && local.is_valid()) {
            instance local_char = local.model_instance();
            if (local_char.is_valid()) {
                uintptr_t primitives[k_esp_primitive_capacity] = {};
                uintptr_t part_addresses[k_esp_primitive_capacity] = {};
                char part_names[k_esp_primitive_capacity][32] = {};
                size_t primitive_count = 0;
                bool has_humanoid = false;
                float health = 0.0f, max_health = 100.0f;
                float rx = 0.0f, ry = 0.0f, rz = 0.0f;
                collect_parts(local_char, primitives, part_addresses, part_names, primitive_count, has_humanoid, health, max_health, rx, ry, rz);
                if (has_humanoid && primitive_count > 0) {
                    bool skel_h = false;
                    SkeletonEntity skel{};
                    collect_skeleton(local_char, skel, skel_h);
                    EspEntity entity{};
                    entity.player_address = local.address;
                    entity.character_address = local_char.address;
                    entity.user_id = read<int64_t>(local.address + Offsets::Player::UserId);
                    entity.is_r15 = skel.is_r15;
                    entity.primitive_count = primitive_count;
                    for (size_t i = 0; i < primitive_count; ++i) {
                        entity.primitives[i] = primitives[i];
                        entity.part_addresses[i] = part_addresses[i];
                        memcpy(entity.part_names[i], part_names[i], 32);
                    }
                    std::string pname = local.get_name();
                    memset(entity.name, 0, sizeof(entity.name));
                    size_t copy_len = pname.size() < 63 ? pname.size() : 63;
                    memcpy(entity.name, pname.c_str(), copy_len);
                    memset(entity.display_name, 0, sizeof(entity.display_name));
                    uintptr_t dn_ptr = read<uintptr_t>(local.address + Offsets::Player::DisplayName);
                    if (is_valid_address(dn_ptr)) {
                        std::string dname = fetchstring(dn_ptr);
                        copy_len = dname.size() < 63 ? dname.size() : 63;
                        memcpy(entity.display_name, dname.c_str(), copy_len);
                    }
                    memset(entity.tool_name, 0, sizeof(entity.tool_name));
                    for (const instance& ch : local_char.get_children()) {
                        if (!ch.is_valid()) continue;
                        if (ch.get_class_name() == "Tool") {
                            std::string tname = ch.get_name();
                            copy_len = tname.size() < 63 ? tname.size() : 63;
                            memcpy(entity.tool_name, tname.c_str(), copy_len);
                            break;
                        }
                    }
                    entity.health = health;
                    entity.max_health = max_health;
                    entity.root_x = rx;
                    entity.root_y = ry;
                    entity.root_z = rz;
                    s_esp_entities.push_back(entity);
                    if (skel.head && skel.upper_torso) {
                        skel.player_address = local.address;
                        s_skeleton_entities.push_back(skel);
                    }
                }
            }
        }
    }

    size_t GetEntityCount() {
        return s_players_with_rig.size();
    }

    const std::vector<uintptr_t>& GetPlayers() {
        return s_players_with_rig;
    }

    const std::vector<EspEntity>& GetEspEntities() {
        return s_esp_entities;
    }

    const std::vector<SkeletonEntity>& GetSkeletonEntities() {
        return s_skeleton_entities;
    }

    const LocalPlayerData& GetLocalPlayer() {
        return s_local_player;
    }

    bool IsPhantomForces() {
        return s_is_phantom_forces;
    }
}
