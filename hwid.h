#pragma once
#include <Windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <intrin.h>

namespace HWID {
    inline std::string GetCPUSerial() {
        int cpuInfo[4] = { 0 };
        __cpuid(cpuInfo, 1);
        
        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(8) << cpuInfo[3]
           << std::setw(8) << cpuInfo[0];
        return ss.str();
    }

    inline std::string GetVolumeSerial() {
        DWORD volumeSerial = 0;
        if (GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0)) {
            std::stringstream ss;
            ss << std::hex << std::setfill('0') << std::setw(8) << volumeSerial;
            return ss.str();
        }
        return "00000000";
    }

    inline std::string GetMachineGUID() {
        HKEY hKey;
        char buffer[256] = { 0 };
        DWORD bufferSize = sizeof(buffer);
        
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
            "SOFTWARE\\Microsoft\\Cryptography", 
            0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
            
            RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, 
                (LPBYTE)buffer, &bufferSize);
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        return "";
    }

    inline std::string GenerateHWID() {
        std::string cpu = GetCPUSerial();
        std::string vol = GetVolumeSerial();
        std::string guid = GetMachineGUID();
        
        std::stringstream hwid;
        hwid << cpu << "-" << vol << "-" << guid;
        
        std::string result = hwid.str();
        
        // Simple hash to obfuscate
        unsigned int hash = 0x811c9dc5;
        for (char c : result) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 0x01000193;
        }
        
        std::stringstream finalHWID;
        finalHWID << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << hash;
        return finalHWID.str();
    }
}
