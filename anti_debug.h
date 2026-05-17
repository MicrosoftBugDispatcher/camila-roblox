#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

namespace AntiDebug {
    inline bool IsDebuggerPresent_Check() {
        return IsDebuggerPresent();
    }

    inline bool CheckRemoteDebugger() {
        BOOL isDebuggerPresent = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent);
        return isDebuggerPresent;
    }

    inline bool CheckProcessList() {
        const std::vector<std::string> blacklistedProcesses = {
            "ollydbg.exe", "x64dbg.exe", "x32dbg.exe", "windbg.exe",
            "ida.exe", "ida64.exe", "idag.exe", "idag64.exe",
            "cheatengine-x86_64.exe", "cheatengine.exe",
            "processhacker.exe", "procmon.exe", "procexp.exe"
        };

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &pe32)) {
            do {
                char processName[MAX_PATH];
                WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, processName, MAX_PATH, nullptr, nullptr);
                
                std::string procNameStr = processName;
                for (auto& c : procNameStr) c = tolower(c);

                for (const auto& blacklisted : blacklistedProcesses) {
                    if (procNameStr.find(blacklisted) != std::string::npos) {
                        CloseHandle(snapshot);
                        return true;
                    }
                }
            } while (Process32NextW(snapshot, &pe32));
        }

        CloseHandle(snapshot);
        return false;
    }

    inline void ProtectProcess() {
        SetUnhandledExceptionFilter(nullptr);
        
        HANDLE hProcess = GetCurrentProcess();
        SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
    }

    inline bool PerformSecurityChecks() {
        if (IsDebuggerPresent_Check()) return false;
        if (CheckRemoteDebugger()) return false;
        if (CheckProcessList()) return false;
        return true;
    }
}
