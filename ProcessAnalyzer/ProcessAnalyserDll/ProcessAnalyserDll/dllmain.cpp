// dllmain.cpp : Определяет точку входа для приложения DLL

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <sstream>
#include <string>

#include "RuntimeDeps.h"
#include "Network.h"
#include "Hooker.h"


static HMODULE g_curMod = nullptr;
static uint64_t counter = 0;

void splitString(const std::string &strlnk, std::vector<std::string> &result, const char delim) {
    if (!result.empty()) result.clear();
    std::stringstream parser(strlnk);
    std::string intermediate;
    while (std::getline(parser, intermediate, delim)) {
        result.push_back(intermediate);
    }
}

DWORD WINAPI routine(_In_ LPVOID lpParameter)
{
    // Start client
    Network ntw;
    auto netBuff = new char[ntw.getBufLen()]{};

    // Connect to the server
    while (ntw.connect() != 0) {
        OutputDebugString(L"Trying to connect");
        Sleep(300);
    }

    OutputDebugString(L"Connected");

    // Send access code
    int iResult = ntw.sendToHost("010101");
    if (iResult <= 0) {
        OutputDebugString(L"Sending error");
        ntw.close();
        FreeLibraryAndExitThread(g_curMod, -1);
    }

    OutputDebugString(L"Sent");

    // Actual routine
    while (true)
    {
        iResult = ntw.recvFromHost(netBuff);
        if (iResult <= 0) break;

        char opByte = netBuff[0];
        
        switch (opByte)
        {
        case Network::GET_RT_DEPS:
        {
            auto info = RuntimeDeps::refillLibInfo();

            // Sending lib number
            std::string numb = char(Network::NUMBER) + std::to_string(info.size());
            if (ntw.sendToHost(numb.c_str()) <= 0 ||
                ntw.recvFlag(Network::NEXT_DATA) < 0)
                break;

            for (const auto& lib : info)
            {
                std::string req = char(Network::LIBRARY) + std::string(lib.path) + '\1' +
                    std::to_string((int)lib.isLocal & (lib.isVirtual<<1));
                
                if (ntw.sendToHost(req.c_str()) <= 0 ||
                    ntw.recvFlag(Network::NEXT_DATA) < 0)
                    break;

                for (int i(0); i < lib.funcs.size();)
                {
                    std::string req{ char(Network::FUNCTION) };

                    while (true) {
                        if (i == lib.funcs.size()) break;

                        std::stringstream add;
                        add << lib.funcs[i].name << '\1' << std::hex <<
                               lib.funcs[i].addr << '\1';

                        if (req.size() + add.str().size() > ntw.getBufLen()) break;

                        req += add.str();
                        ++i;
                    }

                    req.pop_back(); // Removing last space

                    if (ntw.sendToHost(req.c_str()) <= 0 ||
                        ntw.recvFlag(Network::NEXT_DATA) < 0)
                        break;
                }
            }

            ntw.sendFlag(Network::SUCC_FINISHED);

        } break;

        case Network::HOOK_PREV:
        {
            if (ntw.recvFlag(Network::NEXT_DATA) < 0) break;

            if (ntw.recvFromHost(netBuff) <= 0) break;

            std::string injectionInfo(netBuff);
            std::vector<std::string> tokens;
            splitString(injectionInfo, tokens, '\2');

            HMODULE dllHandle = GetModuleHandleA(tokens[0].c_str());
            
            if (!dllHandle) {
                ntw.sendToHost("ERROR");
                break;
            }
            else {
                void* funcAddr = (void*)GetProcAddress(dllHandle, tokens[1].c_str());
                std::stringstream add;

                Hooker hooker;
                uint8_t* instructions;
                hooker.previewHook(funcAddr, nullptr, &instructions);
                add << instructions;
                free(instructions);
                if (ntw.sendToHost(add.str().c_str()) <= 0) break;
            }

            ntw.sendFlag(Network::SUCC_FINISHED);
        } break;

        case Network::HOOK_FUNC:
        {
            if (ntw.recvFlag(Network::NEXT_DATA) < 0) break;

            if (ntw.recvFromHost(netBuff) <= 0) break;

            std::string injectionInfo(netBuff);
            std::vector<std::string> tokens;
            splitString(injectionInfo, tokens, '\2');

            HMODULE targetDll = GetModuleHandleA(tokens[0].c_str());
            if (!targetDll) break;
            void* targetFuncAddr = (void*)GetProcAddress(targetDll, tokens[1].c_str());
            if (!targetFuncAddr) break;


            Hooker hooker;
            uint8_t* bytes;
            hooker.previewHook(targetFuncAddr, &bytes, nullptr);


            HMODULE injDll = LoadLibraryA(tokens[2].c_str());
            if (!injDll) break;
            void* payloadFuncAddr = (void*)GetProcAddress(injDll, tokens[3].c_str());
            if (!payloadFuncAddr) break;


            typedef void** (*trampGetter)();
            //typedef Gdiplus::GpStatus(*trampoline)(GpSolidFill* brush, Gdiplus::ARGB color);
            trampGetter trG = (trampGetter)(GetProcAddress(injDll, "trampGetter"));
            if (!trG) break;
            void** tramp = trG();

            //std::cout << tramp(228);
            
            hooker.installHook(targetFuncAddr, payloadFuncAddr, tramp, &counter);
            std::stringstream add;
            add << std::hex << (char*)bytes;

            if (ntw.sendToHost(add.str().c_str()) <= 0) break;
            ntw.sendFlag(Network::SUCC_FINISHED);
        } break;

        case Network::UNHOOK_FUNC:
        {
            if (ntw.recvFlag(Network::NEXT_DATA) < 0) break;

            if (ntw.recvFromHost(netBuff) <= 0) break;

            std::string ejectionInfo(netBuff);
            std::vector<std::string> tokens;
            splitString(ejectionInfo, tokens, '\2');

            const int result = ntw.recvFromHost(netBuff);
            if (result <= 0) break;
            //
            uint8_t* bytes = new uint8_t[result];
            memcpy(bytes, netBuff, result);
            std::stringstream add;
            add << result;
            ntw.sendToHost(add.str().c_str());
            //

            HMODULE targetDll = GetModuleHandleA(tokens[0].c_str());
            if (!targetDll) break;
            void* targetFuncAddr = (void*)GetProcAddress(targetDll, tokens[1].c_str());
            if (!targetFuncAddr) break;

            HMODULE injDll = GetModuleHandleA(tokens[2].c_str());
            if (!injDll) {
                injDll = LoadLibraryA(tokens[2].c_str());
                if (!injDll) break;
            }
            void* payloadFuncAddr = (void*)GetProcAddress(injDll, tokens[3].c_str());
            if (!payloadFuncAddr) break;

            typedef void** (*trampGetter)();
            //typedef Gdiplus::GpStatus(*trampoline)(GpSolidFill* brush, Gdiplus::ARGB color);
            trampGetter trG = (trampGetter)(GetProcAddress(injDll, "trampGetter"));
            if (!trG) break;
            void** tramp = trG();


            Hooker hooker;
            hooker.uninstallHook(targetFuncAddr, tramp, bytes, result);
            uint8_t* instrs;
            hooker.previewHook(targetFuncAddr, nullptr, &instrs);

            //
            add << instrs;
            free(instrs);
            delete[] bytes;
            ntw.sendToHost(add.str().c_str());
            //

            ntw.sendFlag(Network::SUCC_FINISHED);
        } break;

        default:
            break;
        }
    }

    OutputDebugString(L"Finishing");
    ntw.close();
    RuntimeDeps::clearLibInfo();
    FreeLibraryAndExitThread(g_curMod, 0);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    g_curMod = hModule;
    OutputDebugString(L"Here");

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        if (!CreateThread(NULL, 0, routine, NULL, 0, NULL)) {
            return FALSE;
        }

        break;
    case DLL_THREAD_ATTACH:
        break;
        
    case DLL_THREAD_DETACH:

    case DLL_PROCESS_DETACH:

        break;
    }
    return TRUE;
}

