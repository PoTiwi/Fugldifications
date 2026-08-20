#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "../externals/offsets.hpp"

#if defined(_WIN32)
    #include <windows.h>
    using PlatformHandle = HANDLE;
#elif defined(__linux__)
    #include <sys/types.h>
    #include <sys/uio.h>
    using PlatformHandle = pid_t; 
#elif defined(__APPLE__)
    #include <mach/mach.h>
    using PlatformHandle = mach_port_t; 
#endif

class Memory {
    PlatformHandle hProcess;
    uintptr_t base;
public:
    Memory() : base(0) {
#if defined(_WIN32) || defined(__APPLE__)
        hProcess = 0;
#elif defined(__linux__)
        hProcess = -1;
#endif
    }
    Memory(PlatformHandle hProc, uintptr_t baseAddress) : hProcess(hProc), base(baseAddress) {}

    uintptr_t point_addr(uintptr_t parentAddress, uintptr_t offset);

    template <typename T>
    T defset(uintptr_t parentAddress, uintptr_t offset) {
        return read<T>(parentAddress + offset);
    }

    template <typename T>
    T read(uintptr_t address) {
        T buffer{};
        if (!address) return buffer;

#if defined(_WIN32)
        if (hProcess) {
            ReadProcessMemory(hProcess, (LPCVOID)address, &buffer, sizeof(T), NULL);
        }
#elif defined(__linux__)
        if (hProcess > 0) {
            struct iovec local[1];
            struct iovec remote[1];
            local[0].iov_base = &buffer;
            local[0].iov_len = sizeof(T);
            remote[0].iov_base = (void*)address;
            remote[0].iov_len = sizeof(T);
            process_vm_readv(hProcess, local, 1, remote, 1, 0);
        }
#elif defined(__APPLE__)
        if (hProcess) {
            vm_size_t size = sizeof(T);
            vm_read_overwrite(hProcess, (vm_address_t)address, size, (vm_address_t)&buffer, &size);
        }
#endif
        return buffer;
    }

    template <typename T>
    void overwrite(uintptr_t address, T value) {
        if (!address) return;

#if defined(_WIN32)
        if (hProcess) {
            WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);
        }
#elif defined(__linux__)
        if (hProcess > 0) {
            struct iovec local[1];
            struct iovec remote[1];
            local[0].iov_base = &value;
            local[0].iov_len = sizeof(T);
            remote[0].iov_base = (void*)address;
            remote[0].iov_len = sizeof(T);
            process_vm_writev(hProcess, local, 1, remote, 1, 0);
        }
#elif defined(__APPLE__)
        if (hProcess) {
            mach_vm_write(hProcess, (mach_vm_address_t)address, (vm_offset_t)&value, sizeof(T));
        }
#endif
    }
};

extern PlatformHandle g_hProcess;
extern Memory Imem;
extern uintptr_t g_workspace;
extern uintptr_t g_world;
extern uintptr_t game;
extern bool g_isRunning;

#define Instance uintptr_t // just to make it more readable
#define while_instance_running while(g_isRunning) for(bool _once = true; _once; _once = false, Sleep(10)) // custom while lloop, so you wont need Sleep, etc

bool init();
void cleanup();
bool checkExitKey();
void sleep_ms(int ms);
