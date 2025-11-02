#include "pch.h"
#include "hooks.h"
#include "features.h"
#include "Instances.hpp"

namespace Memory
{
    uintptr_t FindPattern(const char* Signature)
    {
        static auto PatternToByte = [](const char* Pattern)
            {
                auto Bytes = std::vector<int>{};
                const auto Start = const_cast<char*>(Pattern);
                const auto End = const_cast<char*>(Pattern) + strlen(Pattern);

                for (auto Current = Start; Current < End; ++Current)
                {
                    if (*Current == '?')
                    {
                        ++Current;
                        if (*Current == '?')
                            ++Current;
                        Bytes.push_back(-1);
                    }
                    else
                    {
                        Bytes.push_back(strtoul(Current, &Current, 16));
                    }
                }
                return Bytes;
            };

        const auto DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
        const auto NtHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)DosHeader + DosHeader->e_lfanew);

        const auto SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
        auto PatternBytes = PatternToByte(Signature);
        const auto ScanBytes = reinterpret_cast<std::uint8_t*>(DosHeader);

        const auto s = PatternBytes.size();
        const auto d = PatternBytes.data();

        for (auto i = 0ul; i < SizeOfImage - s; ++i)
        {
            bool Found = true;
            for (auto j = 0ul; j < s; ++j)
            {
                if (ScanBytes[i + j] != d[j] && d[j] != -1)
                {
                    Found = false;
                    break;
                }
            }
            if (Found)
            {
                return (uintptr_t)&ScanBytes[i];
            }
        }
        return NULL;
    }
}

DWORD WINAPI Thread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "[+] Injected! Initializing SDK..." << std::endl;

    // --- SDK Initialization
    uintptr_t GObjectsAddress = Memory::FindPattern("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1"); //or Offsets::GObjects 
    if (!GObjectsAddress) {
        std::cout << "[-] Failed to find GObjects pattern!" << std::endl;
        if (f) fclose(f); FreeConsole(); FreeLibraryAndExitThread(hModule, 1);
        return 1;
    }
    GObjectsAddress = GObjectsAddress + *reinterpret_cast<int32_t*>(GObjectsAddress + 3) + 7;
    UObject::GObjects.InitManually(reinterpret_cast<void*>(GObjectsAddress));

    if (!UObject::GObjects.GetTypedPtr()) {
        std::cout << "[-] GObjects pointer is null after manual initialization!" << std::endl;
        if (f) fclose(f); FreeConsole(); FreeLibraryAndExitThread(hModule, 1);
        return 1;
    }
    std::cout << "[+] SDK Initialized! Found " << std::dec << UObject::GObjects->Num() << " objects." << std::endl;

    Instances::Initialize();
    Hooks::Initialize();

    /*
    for (int i = 0; i < UObject::GObjects->NumElements; i++)
    {
        auto addr = GetModuleHandle(NULL) + 0x3B9E8B8;
        if (UObject::GObjects->GetByIndex(i) != nullptr && UObject::GObjects->GetByIndex(i)->VTable == (void*)addr)
        {
            auto name = UObject::GObjects->GetByIndex(i)->Name;
            std::cout << name.ToString() << std::endl;
        }
    }
    */

    std::cout << "[+] -> Press END to eject." << std::endl;
    while (!(GetAsyncKeyState(VK_END) & 1) && !Features::g_Unload)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[+] Ejecting..." << std::endl;
    Hooks::Shutdown();

    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);

    return 0;
}

// DLL entry point (unchanged).
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Thread, hModule, 0, nullptr);
    }
    return TRUE;
}