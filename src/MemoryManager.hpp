#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>

struct ModuleInfo {
    uintptr_t base = 0;
    size_t size = 0;
};

class MemoryManager {
private:
    DWORD processId = 0;
    HANDLE hProcess = nullptr;

public:
    MemoryManager() = default;
    ~MemoryManager() {
        CleanUp();
    }

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    bool IsAttached() const {
        return hProcess != nullptr;
    }

    bool Attach(const std::wstring& processName) {
        CleanUp();

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    this->processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);

        if (this->processId == 0) return false;

        this->hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, this->processId);
        return (this->hProcess != nullptr);
    }

    ModuleInfo GetModuleInfo(const std::wstring& moduleName) {
        ModuleInfo info;
        if (this->processId == 0) return info;

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processId);
        if (hSnapshot == INVALID_HANDLE_VALUE) return info;

        MODULEENTRY32W moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32W);

        if (Module32FirstW(hSnapshot, &moduleEntry)) {
            do {
                if (moduleName == moduleEntry.szModule) {
                    info.base = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
                    info.size = moduleEntry.modBaseSize;
                    break;
                }
            } while (Module32NextW(hSnapshot, &moduleEntry));
        }

        CloseHandle(hSnapshot);
        return info;
    }

    // Reads `size` raw bytes into `buffer`; used for strings and other variable-length data.
    bool ReadBytes(uintptr_t address, void* buffer, size_t size) {
        if (hProcess == nullptr || address == 0) return false;
        SIZE_T got = 0;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &got) && got == size;
    }

    template <typename T>
    T Read(uintptr_t address) {
        T buffer{};
        if (hProcess == nullptr || address == 0) return buffer;

        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), nullptr);
        return buffer;
    }

    // Scans [start, end) for a byte pattern; -1 in `pattern` marks a wildcard byte.
    // Only committed, executable pages are searched, matching how code signatures are located.
    uintptr_t FindPattern(uintptr_t start, uintptr_t end, const std::vector<int>& pattern) {
        if (hProcess == nullptr || pattern.empty() || start >= end) return 0;

        std::vector<uint8_t> buf;
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t cursor = start;

        while (cursor < end && VirtualQueryEx(hProcess, (LPCVOID)cursor, &mbi, sizeof(mbi))) {
            uintptr_t regionBase = (uintptr_t)mbi.BaseAddress;
            size_t regionSize = mbi.RegionSize;

            if (mbi.State == MEM_COMMIT &&
                (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) &&
                !(mbi.Protect & PAGE_GUARD))
            {
                size_t readSize = regionSize;
                if (regionBase + readSize > end) readSize = (size_t)(end - regionBase);

                if (buf.size() < readSize) buf.resize(readSize);
                SIZE_T got = 0;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buf.data(), readSize, &got) && got >= pattern.size()) {
                    for (size_t i = 0; i + pattern.size() <= got; i++) {
                        bool match = true;
                        for (size_t j = 0; j < pattern.size(); j++) {
                            if (pattern[j] != -1 && buf[i + j] != (uint8_t)pattern[j]) { match = false; break; }
                        }
                        if (match) return regionBase + i;
                    }
                }
            }

            cursor = regionBase + regionSize;
        }

        return 0;
    }

    // Safely closes any lingering OS handles
    void CleanUp() {
        if (hProcess) {
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
        processId = 0;
    }
};