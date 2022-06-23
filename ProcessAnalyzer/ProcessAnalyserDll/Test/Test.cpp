#include <Windows.h>
#include <gdiplus.h>
#include <Psapi.h>
#include "Hooker.h"
#pragma comment (lib, "Gdiplus.lib")


//DWORD(*tramp)(DWORD ms);
DWORD trump(DWORD ms) {
	return ms;
}
Gdiplus::GpStatus(*tramp)(Gdiplus::GpSolidFill* brush, Gdiplus::ARGB color);// = trump;
//HMODULE(*tramp)(LPCSTR lpLibFileName);
extern "C" _declspec(dllexport)
void tickHooker(DWORD ms)
{
	OutputDebugString(L"Hello sleep");
	//tramp(ms);
	return;
}

extern "C" _declspec(dllexport)
Gdiplus::GpStatus GdipSetSolidFillColorPayload(Gdiplus::GpSolidFill* brush, Gdiplus::ARGB color)
{
	Gdiplus::ARGB orange = 0xffff7700;
	return tramp(brush, orange);
}

extern "C" _declspec(dllexport)
void** trampGetter() {
	//return;
	return (void**)&tramp;
}

//extern "C" _declspec(dllexport)
//HMODULE LoadLibraryAPayload(LPCSTR lpLibFileName) {
//	return tramp("D:\\unbeatable\\Library.dll");
//}


DWORD WINAPI routine(void* param)
{
	OutputDebugString(L"In the routine\n");

	auto lib = LoadLibrary(L"Kernel32.dll");
	if (!lib) {
		OutputDebugString(L"Fuck lib\n");
		return -1;
	}

	void* func = GetProcAddress(lib, "GetTickCount");
	if (!func) {
		OutputDebugString(L"Fuck func\n");
		return -2;
	}

	OutputDebugString(L"Ready to hook\n");

	uint64_t counter;
	Hooker hk;

	hk.installHook(func, tickHooker, (void**)&tramp, &counter);

	OutputDebugString(L"Hooked\n");

	while (true) {
		printf("%I64u", counter);
		Sleep(1000);
	}
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	OutputDebugString(L"Here\n");

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		//if (!CreateThread(NULL, 0, routine, NULL, 0, NULL)) {
		//	return FALSE;
		//}

		break;
	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:

	case DLL_PROCESS_DETACH:

		break;
	}
	return TRUE;
}
