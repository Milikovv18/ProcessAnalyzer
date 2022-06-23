#include "injector.h"


Injector::Injector()
{

}


void Injector::apply(int procId, const char* dllName)
{
    qDebug() << procId;
    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, false, procId);

    BOOL is32;
    IsWow64Process(h, &is32);
    qDebug() << is32;

    if (h)
    {
        HMODULE hMod = GetModuleHandleA("kernel32.dll");
        if (!hMod) {
            throw "Wtf happened here";
        }

        LPVOID LoadLibAddr = (LPVOID)GetProcAddress(hMod, "LoadLibraryA");
        if (!LoadLibAddr) {
            throw "Wtf happened here";
        }

        LPVOID dereercomp = VirtualAllocEx(h, NULL, strlen(dllName), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!dereercomp) {
            throw "Smth wrong with virtual memory";
        }

        int res = WriteProcessMemory(h, dereercomp, dllName, strlen(dllName), NULL);
        if (res == 0) {
            throw "Cant write into process mem";
        }

        qDebug() << "creating Thread";
        HANDLE asdc = CreateRemoteThread(h, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, dereercomp, 0, NULL);
        if (!asdc) {
            throw "Cant create remote thread";
        }

        res = WaitForSingleObject(asdc, INFINITE);
        if (res == WAIT_FAILED) {
            throw "I cant wait!!!";
        }

        DWORD exitCode(0);
        GetExitCodeThread(asdc, &exitCode);
        qDebug() << exitCode;

        VirtualFreeEx(h, dereercomp, NULL, MEM_RELEASE);
        CloseHandle(asdc);
        qDebug() << "Closed handle\n" << "ASDC: " << asdc;
        CloseHandle(h);
    }
}
