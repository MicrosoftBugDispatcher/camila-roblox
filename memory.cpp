#include "memory.h"
#include "offsets.h"

MemoryClass g_memory;

namespace mem {
    std::atomic<HANDLE> roblox_h{ nullptr };
    std::atomic<uint32_t> process_id{ 0 };
}

bool is_valid_address(uintptr_t address) {
    return address != 0 && address >= 0x10000 && address < 0x7FFFFFFFFFFF;
}

static std::string read_string_raw(uint64_t address) {
    std::string s;
    s.reserve(204);
    char buf[200];
    if (!g_memory.ReadRaw(address, buf, sizeof(buf))) return s;
    for (int i = 0; i < 200; ++i) {
        if (buf[i] == 0) break;
        s.push_back(buf[i]);
    }
    return s;
}

std::string fetchstring(uint64_t address) {
    int length = read<int>(address + 0x18);
    if (length >= 16u) {
        uintptr_t padding = read<uintptr_t>(address);
        return read_string_raw(padding);
    }
    return read_string_raw(address);
}

std::string instance::get_name() const {
    return fetchstring(read<uintptr_t>(address + Offsets::Instance::Name));
}

std::string instance::get_class_name() const {
    return fetchstring(read<uintptr_t>(read<uintptr_t>(address + Offsets::Instance::ClassDescriptor) + Offsets::Instance::ClassName));
}

instance instance::read_child(const std::string& child_name) const {
    for (auto child : get_children()) {
        if (child.get_name() == child_name) return child;
    }
    return instance{};
}

instance instance::model_instance() const {
    return read<instance>(address + Offsets::Player::ModelInstance);
}

instance instance::local_player() const {
    return read<instance>(address + Offsets::Player::LocalPlayer);
}

std::vector<instance> instance::get_children() const {
    std::vector<instance> children;
    uint64_t begin = read<uint64_t>(address + Offsets::Instance::ChildrenStart);
    uint64_t end = read<uint64_t>(begin + Offsets::Instance::ChildrenEnd);
    if (!begin) return children;
    if (!end) return children;
    for (auto instances = read<uint64_t>(begin); instances != end; instances += 16u) {
        children.emplace_back(read<instance>(instances));
    }
    return children;
}

instance instance::read_service(const std::string& service_name) const {
    instance returned{};
    for (auto child : get_children()) {
        if (child.get_class_name() == service_name) return child;
    }
    return returned;
}

bool read_raw(uint64_t address, void* buffer, size_t size) {
    if (!is_valid_address(address)) return false;
    return g_memory.ReadRaw(address, buffer, size);
}

bool write_raw(uint64_t address, const void* data, size_t size) {
    if (!is_valid_address(address)) return false;
    return g_memory.WriteRaw(address, data, size);
}

bool mem::grabroblox_h() {
    DWORD access = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION;
    uint32_t target_pid = process_id.load();
    HANDLE h = OpenProcess(access, FALSE, target_pid);
    if (h && h != INVALID_HANDLE_VALUE) {
        HANDLE old = roblox_h.exchange(h);
        if (old && old != INVALID_HANDLE_VALUE)
            CloseHandle(old);
        g_memory.Handle = roblox_h.load();
        return true;
    }
    return false;
}
