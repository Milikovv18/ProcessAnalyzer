#pragma once
#define WIN32_LEAN_AND_MEAN

#include <vector>

#include <Windows.h>
#include <Psapi.h>
#include <ImageHlp.h>
#include <tlhelp32.h>
#include <cstdlib>


class RuntimeDeps
{
public:
    struct FuncInfo
    {
        char* name = nullptr;
        LONG_PTR addr;
    };

    struct LibInfo
    {
        char path[256]{};
        std::vector<FuncInfo> funcs;
        HMODULE mod = nullptr;

        bool isLocal = false;
        bool isVirtual = false;
        bool exports = true;
    };

    static const std::vector<LibInfo> refillLibInfo();
    static void clearLibInfo();

private:
    inline static std::vector<LibInfo> m_libsInfo;

    static void fillFuncInfo(LibInfo& info);
};