#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

namespace cache {
    inline constexpr size_t k_esp_primitive_capacity = 16;

    struct EspEntity {
        uintptr_t player_address;
        uintptr_t character_address;
        int64_t user_id;
        bool is_r15;
        uintptr_t primitives[k_esp_primitive_capacity];
        uintptr_t part_addresses[k_esp_primitive_capacity];
        char part_names[k_esp_primitive_capacity][32];
        size_t primitive_count;
        char name[64];
        char display_name[64];
        char tool_name[64];
        float health;
        float max_health;
        float root_x, root_y, root_z;
    };

    struct LocalPlayerData {
        float x, y, z;
        bool valid;
        uintptr_t humanoid_address;
        uintptr_t hrp_primitive;
    };

    struct SkeletonEntity {
        uintptr_t player_address;
        uintptr_t head;
        uintptr_t upper_torso;
        uintptr_t lower_torso;
        uintptr_t left_upper_arm;
        uintptr_t right_upper_arm;
        uintptr_t left_lower_arm;
        uintptr_t right_lower_arm;
        uintptr_t left_hand;
        uintptr_t right_hand;
        uintptr_t left_upper_leg;
        uintptr_t right_upper_leg;
        uintptr_t left_lower_leg;
        uintptr_t right_lower_leg;
        uintptr_t left_foot;
        uintptr_t right_foot;
        uintptr_t pf_limbs[5];
        bool is_r15;
        bool is_phantom_forces;
    };

    void Update(uintptr_t base);
    bool IsPhantomForces();
    size_t GetEntityCount();
    const std::vector<uintptr_t>& GetPlayers();
    const std::vector<EspEntity>& GetEspEntities();
    const std::vector<SkeletonEntity>& GetSkeletonEntities();
    const LocalPlayerData& GetLocalPlayer();
}
