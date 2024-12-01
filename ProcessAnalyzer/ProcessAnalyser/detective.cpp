#include "detective.h"


Detective::Detective(quint64 bufLen)
{
    m_uniBufLen = bufLen;
    m_uniBuffer = new char[bufLen]{};
}


Detective::~Detective()
{
    delete[] m_uniBuffer;
}




QList<ProcInfo> Detective::getRunningProcesses()
{
    QList<ProcInfo> procs;

    // Take a snapshot of all processes in the system.
    HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE ) {
        throw "Bad your snapshot";
    }

    // Set the size of the structure before using it.
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof( PROCESSENTRY32 );

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if(!Process32First(hProcessSnap, &pe32)) {
        throw "Something bad";
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if ( hProcess == NULL ) {
           continue;
        }

        clear_buf;

        unsigned long pathSize = 256;
        QueryFullProcessImageName(hProcess, NULL, (wchar_t*)m_uniBuffer, &pathSize);

        procs.append(ProcInfo{ (int)pe32.th32ProcessID, QString::fromWCharArray((wchar_t*)m_uniBuffer) });

    } while( Process32Next( hProcessSnap, &pe32 ) );

    CloseHandle( hProcessSnap );

    return procs;
}


QList<ModuleObject> Detective::getStaticImport(QString fileName)
{
    clear_buf;
    fileName.toWCharArray((wchar_t*)m_uniBuffer);
    HANDLE handle = CreateFile((wchar_t*)m_uniBuffer, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!handle) {
        throw "Bad create file";
    }

    DWORD byteread, size = GetFileSize(handle, NULL);
    PVOID virtualpointer = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (!virtualpointer) {
        throw "Bad virtual pointer";
    }

    if (!ReadFile(handle, virtualpointer, size, &byteread, NULL)) {
        throw "Bad read file";
    }

    CloseHandle(handle);

    // Get pointer to NT header
    PIMAGE_NT_HEADERS        ntheaders =
            (PIMAGE_NT_HEADERS)(PCHAR(virtualpointer) +PIMAGE_DOS_HEADER(virtualpointer)->e_lfanew);
    PIMAGE_SECTION_HEADER    pSech = IMAGE_FIRST_SECTION(ntheaders);//Pointer to first section header
    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor; //Pointer to import descriptor

    QList<ModuleObject> toRet;
    QStringList helpList;

    if (ntheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size != 0)
    {
        pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)virtualpointer +
            Rva2Offset(ntheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                       pSech, ntheaders->FileHeader.NumberOfSections));

        // Walk until you reached an empty IMAGE_IMPORT_DESCRIPTOR
        while (pImportDescriptor->Name != NULL)
        {
            toRet.append(ModuleObject());
            auto& lastLib = toRet.last();

            // Get the name of each dll
            auto name = QString((PCHAR)((DWORD_PTR)virtualpointer +
                                        Rva2Offset(pImportDescriptor->Name,
                                                   pSech,
                                                   ntheaders->FileHeader.NumberOfSections)));

            // Check that this library is not already in the list
            helpList.append(name);

            // Get the path to this dll
            auto path = getLibPath(name);
            lastLib.setPath(path);

            // Check if the dll is local (or get the exact path)
            if (Helper::getProcessPath((HANDLE)-1).toLower() == path.toLower())
            {
                // Try to clarify the path
                path = getLibPath(name, true);
                lastLib.setPath(path);

                if (path.isEmpty()) {
                    lastLib.setName(name);
                    lastLib.setLocal(true);
                }
            }

            // Check if the dll is virtual
            if (lastLib.getName().toLower() != name.toLower())
            {
                lastLib.setName(name);
                lastLib.setVirtual(true);
            }

            pImportDescriptor++;
        }

    }
    else {
        return QList<ModuleObject>();
    }


    if (virtualpointer)
        VirtualFree(virtualpointer, NULL, MEM_RELEASE);

    return toRet;
}


QStringList Detective::getStaticExport(ModuleObject& obj)
{
    unsigned long cDirSize;
    _LOADED_IMAGE LoadedImage;
    QStringList toRet;

    if (!MapAndLoad(obj.getPath().toStdString().c_str(), NULL, &LoadedImage, TRUE, TRUE)) {
        obj.setExports(false);
        return toRet;
    }

    _IMAGE_EXPORT_DIRECTORY* ImageExportDirectory = (_IMAGE_EXPORT_DIRECTORY*)
            ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                      false, IMAGE_DIRECTORY_ENTRY_EXPORT,
                                      &cDirSize);

    if (ImageExportDirectory != NULL)
    {
        DWORD* dNameRVAs = (DWORD*)
                ImageRvaToVa(LoadedImage.FileHeader,
                             LoadedImage.MappedAddress,
                             ImageExportDirectory->AddressOfNames, NULL);

        for(size_t i = 0; i < ImageExportDirectory->NumberOfNames; i++)
        {
            QString sName = (char*)
                    ImageRvaToVa(LoadedImage.FileHeader,
                                 LoadedImage.MappedAddress,
                                 dNameRVAs[i], NULL);
            toRet.append(sName);
        }
    }
    UnMapAndLoad(&LoadedImage);

    if (toRet.isEmpty()) {
        obj.setExports(false);
    }

    return toRet;
}


QList<ModuleObject> Detective::getRuntimeDeps(int procId)
{
    HANDLE handl = OpenProcess(PROCESS_ALL_ACCESS, TRUE, procId);
    if (!handl) {
        throw "Bad access level";
    }

    unsigned long actually(0);
    HMODULE mods[256]{};
    if (!EnumProcessModules(handl, mods, 256, &actually)) {
        throw "Bad modules";
    }


    // Get current path (for speed)
    auto procPath = Helper::getProcessPath(handl);
    auto procPathArr = new wchar_t[256]{};
    procPath.toWCharArray(procPathArr);

    // Get process paths
    clear_buf;
    QList<ModuleObject> toRet;
    for (unsigned int i(0); i < actually; ++i)
    {
        // If the module path is the current process path (path not found)
        GetModuleFileNameEx(handl, mods[i], (wchar_t*)m_uniBuffer, 256);
        if (_wcsicmp(procPathArr, (wchar_t*)m_uniBuffer) == 0)
            continue;

        auto path = QString::fromWCharArray((wchar_t*)m_uniBuffer);
        auto name = Helper::getNameFromPath(path);

        toRet.append(ModuleObject(path, name));
    }

    delete[] procPathArr;

    return toRet;
}


QString Detective::getLibPath(QString libName, bool system)
{
    if (system) {
        return getLibPathExact(libName);
    } else {
        return getLibPathRough(libName);
    }
}

QString Detective::getLibPathRough(QString libName)
{
    clear_buf;

    // The fastest transformation
    std::string winDllName = libName.toStdString();
    if (!GetModuleFileNameA(GetModuleHandleA(winDllName.c_str()), m_uniBuffer, 256))
    {
        throw "Bad lib";
    }

    return QString::fromLocal8Bit(m_uniBuffer);
}

QString Detective::getLibPathExact(QString libName)
{
    QString toRet;
    std::string tempLibName = libName.toStdString();

    HANDLE lib = LoadLibraryA(tempLibName.c_str());
    if (!lib) {
        // If the library is not system
        return "";
    }

    HMODULE mod = GetModuleHandleA(tempLibName.c_str());
    if (!mod) {
        throw "Bad module";
    }

    clear_buf;
    if (!GetModuleFileNameExA(GetCurrentProcess(), mod, m_uniBuffer, 256)) {
        throw "Bad filename";
    }
    toRet = QString::fromLocal8Bit(m_uniBuffer);

    if (!FreeLibrary(mod)) {
        throw "Unable to free library";
    }

    return toRet;
}


DWORD Detective::Rva2Offset(DWORD rva, PIMAGE_SECTION_HEADER psh, unsigned numOfSections) const
{
    size_t i = 0;
    PIMAGE_SECTION_HEADER pSeh;
    if (rva == 0)
    {
        return (rva);
    }

    pSeh = psh;
    for (i = 0; i < numOfSections; i++)
    {
        if (rva >= pSeh->VirtualAddress && rva < pSeh->VirtualAddress +
            pSeh->Misc.VirtualSize)
        {
            break;
        }
        pSeh++;
    }

    return (rva - pSeh->VirtualAddress + pSeh->PointerToRawData);
}




