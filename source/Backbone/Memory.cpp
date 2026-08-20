#include "./Memory.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <string>

#if defined(_WIN32)
    #include <tlhelp32.h>
#elif defined(__linux__)
    #include <dirent.h>
    #include <unistd.h>
    #include <fstream>
    #include <sstream>
    using DWORD = uint32_t;
#elif defined(__APPLE__)
    #include <libproc.h>
    #include <mach/mach_vm.h>
    #include <mach/mach_init.h>
    #include <ApplicationServices/ApplicationServices.h>
    using DWORD = uint32_t;
#endif

uintptr_t Memory::point_addr(uintptr_t parentAddress, uintptr_t offset) {
    return parentAddress + offset;
}

#if defined(_WIN32)
    PlatformHandle g_hProcess = nullptr;
#elif defined(__linux__)
    PlatformHandle g_hProcess = -1;
#elif defined(__APPLE__)
    PlatformHandle g_hProcess = 0;
#endif

Memory Imem;
uintptr_t g_workspace = 0;
uintptr_t g_world = 0;
uintptr_t game = 0;
bool g_isRunning = false;

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool checkExitKey() {
#if defined(_WIN32)
    return (GetAsyncKeyState(0x1B) & 0x8000);
#elif defined(__APPLE__)
    return CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 119);
#elif defined(__linux__)
    return false;
#endif
}

DWORD GetRobloxPID() {
    DWORD pid = 0;
#if defined(_WIN32)
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(entry);
        if (Process32First(snapshot, &entry)) {
            do {
                if (std::string(entry.szExeFile) == "RobloxPlayerBeta.exe") {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
#elif defined(__linux__)
    DIR* dir = opendir("/proc");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                int currentPid = atoi(entry->d_name);
                if (currentPid > 0) {
                    std::string cmdPath = std::string("/proc/") + entry->d_name + "/comm";
                    std::ifstream cmdFile(cmdPath);
                    std::string commName;
                    if (cmdFile >> commName) {
                        if (commName.find("RobloxPlayer") != std::string::npos) {
                            pid = currentPid;
                            break;
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
#elif defined(__APPLE__)
    int pids[1024];
    int count = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
    for (int i = 0; i < count / (int)sizeof(int); i++) {
        if (pids[i] == 0) continue;
        char name[256];
        proc_name(pids[i], name, sizeof(name));
        if (std::string(name).find("RobloxPlayer") != std::string::npos) {
            pid = pids[i];
            break;
        }
    }
#endif
    return pid;
}

uintptr_t GetBaseAddress(DWORD pid) {
    uintptr_t base = 0;
#if defined(_WIN32)
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 entry;
        entry.dwSize = sizeof(entry);
        if (Module32First(snapshot, &entry)) {
            base = (uintptr_t)entry.modBaseAddr;
        }
        CloseHandle(snapshot);
    }
#elif defined(__linux__)
    std::string mapsPath = std::string("/proc/") + std::to_string(pid) + "/maps";
    std::ifstream mapsFile(mapsPath);
    std::string line;
    if (std::getline(mapsFile, line)) {
        size_t dashPos = line.find('-');
        if (dashPos != std::string::npos) {
            std::string baseStr = line.substr(0, dashPos);
            base = std::stoull(baseStr, nullptr, 16);
        }
    }
#elif defined(__APPLE__)
    mach_port_t task;
    if (task_for_pid(mach_task_self(), pid, &task) == KERN_SUCCESS) {
        mach_vm_address_t address = 0;
        mach_vm_size_t size;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object_name;
        
        kern_return_t kr = mach_vm_region(task, &address, &size, VM_REGION_BASIC_INFO_64, 
                                        (vm_region_info_t)&info, &count, &object_name);
        if (kr == KERN_SUCCESS) {
            base = (uintptr_t)address;
        }
    }
#endif
    return base;
}

bool init() {
    DWORD pid = GetRobloxPID(); 
    if (pid == 0) {
        std::cerr << "Could not find Roblox process. Make sure the game is running.\n";
        return false;
    }

#if defined(_WIN32)
    g_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); 
    if (!g_hProcess) {
        std::cerr << "Failed to open process. Try running as Administrator.\n";
        return false;
    }
#elif defined(__linux__)
    g_hProcess = pid;
#elif defined(__APPLE__)
    if (task_for_pid(mach_task_self(), pid, &g_hProcess) != KERN_SUCCESS) {
        std::cerr << "Failed to get macOS task port. Tool must run with sudo or entitlements.\n";
        return false;
    }
#endif

    uintptr_t base = GetBaseAddress(pid); 
    if (base == 0) {
        std::cerr << "Failed to get module base address.\n";
        cleanup();
        return false;
    }

    Imem = Memory(g_hProcess, base); 

    uintptr_t fakeDataModel = Imem.read<uintptr_t>(base + Offsets::FakeDataModel::Pointer);
    if (!fakeDataModel) {
        std::cerr << "Invalid FakeDataModel pointer. Has the game updated?\n";
        cleanup();
        return false;
    }

    uintptr_t realDataModel = Imem.read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel);
    if (!realDataModel) {
        std::cerr << "Invalid RealDataModel pointer.\n";
        cleanup();
        return false;
    }
    
    g_workspace = Imem.read<uintptr_t>(realDataModel + Offsets::DataModel::Workspace);
    if (!g_workspace) {
        std::cerr << "Invalid Workspace pointer.\n";
        cleanup();
        return false;
    }

    g_world = Imem.read<uintptr_t>(g_workspace + Offsets::Workspace::World);
    if (!g_world) {
        std::cerr << "Invalid World pointer.\n";
        cleanup();
        return false;
    }

    std::cout << "Successfully linked to game memory structures.\n";
    g_isRunning = true;
    return true;
}

void cleanup() {
#if defined(_WIN32)
    if (g_hProcess) {
        CloseHandle(g_hProcess);
        g_hProcess = nullptr;
    }
#elif defined(__linux__)
    g_hProcess = -1;
#elif defined(__APPLE__)
    if (g_hProcess) {
        mach_port_deallocate(mach_task_self(), g_hProcess);
        g_hProcess = 0;
    }
#endif
    g_isRunning = false;
    std::cout << "Cleaned up process handles. Exited safely.\n";
}
