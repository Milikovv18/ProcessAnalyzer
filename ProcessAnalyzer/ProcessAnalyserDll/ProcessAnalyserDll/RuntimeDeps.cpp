#include "RuntimeDeps.h"


void RuntimeDeps::fillFuncInfo(LibInfo& info)
{
    _LOADED_IMAGE LoadedImage;
    if (!MapAndLoad(info.path, NULL, &LoadedImage, TRUE, TRUE)) {
        info.exports = false;
        return;
    }

    unsigned long cDirSize;
    _IMAGE_EXPORT_DIRECTORY* ImageExportDirectory = (_IMAGE_EXPORT_DIRECTORY*)
        ImageDirectoryEntryToData(LoadedImage.MappedAddress,
            false, IMAGE_DIRECTORY_ENTRY_EXPORT,
            &cDirSize);

    if (!ImageExportDirectory || ImageExportDirectory->NumberOfNames == 0)
    {
        info.exports = false;
        UnMapAndLoad(&LoadedImage);
        return;
    }

    DWORD* dNameRVAs = (DWORD*)ImageRvaToVa(LoadedImage.FileHeader,
        LoadedImage.MappedAddress,
        ImageExportDirectory->AddressOfNames, NULL);

    for (size_t i = 0; i < ImageExportDirectory->NumberOfNames; i++)
    {
        auto tempChar = (char*)ImageRvaToVa(LoadedImage.FileHeader,
            LoadedImage.MappedAddress,
            dNameRVAs[i], NULL);

        info.funcs.push_back(FuncInfo{});
        info.funcs.back().name = new char[strlen(tempChar)]{};
        memcpy(info.funcs.back().name, tempChar, strlen(tempChar) + 1); // +'\0'
        info.funcs.back().addr = (LONG_PTR)GetProcAddress(info.mod, info.funcs[i].name);
    }

    UnMapAndLoad(&LoadedImage);
}


const std::vector<RuntimeDeps::LibInfo> RuntimeDeps::refillLibInfo()
{
    clearLibInfo();

    auto handle = GetCurrentProcess();
    auto curPath = new char[256]{};
    GetModuleFileNameA(NULL, curPath, 256);

    // Get all modules (the size is unknown)
    unsigned long actually(0);
    HMODULE* mods = new HMODULE;
    if (!EnumProcessModules(handle, mods, 1, &actually)) {
        return std::vector<RuntimeDeps::LibInfo>();
    }
    delete mods;
    mods = new HMODULE[actually]{};
    EnumProcessModules(handle, mods, actually, &actually);

    // Fill up LibInfo
    auto path = new char[256]{};
    for (unsigned int i(0); i < actually; ++i)
    {
        GetModuleFileNameExA(handle, mods[i], path, 256);
        if (_stricmp(path, curPath) == 0)
            continue;

        bool isExists = false;
        for (auto& lib : m_libsInfo)
            if (_stricmp(lib.path, path) == 0) {
                isExists = true;
                break;
            }
        if (isExists) continue;

        LibInfo obj;
        memcpy(obj.path, path, 256);
        obj.mod = mods[i];
        fillFuncInfo(obj);

        m_libsInfo.push_back(obj);
    }

    delete[] mods;
    return m_libsInfo;
}


void RuntimeDeps::clearLibInfo()
{
    for (auto& lib : m_libsInfo) {
        lib.funcs.clear();
    }
    m_libsInfo.clear();
}