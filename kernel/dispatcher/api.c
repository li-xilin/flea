#include "kernel/api.h"
#include "kernel/flea.h"

#define set(api, hmodule, symbol) if (!(*(UINT_PTR*)&(api)->symbol = (UINT_PTR)(api)->GetProcAddress(hmodule, STATIC_STR( #symbol )))) goto out

INT InitializeWin32Api(LPWIN32API pApi)
{
	INT iRetVal = -1;
	HMODULE hKernel32 = pApi->LoadLibraryA(STATIC_STR("kernel32.dll"));
	HMODULE hWs2_32 = pApi->LoadLibraryA(STATIC_STR("ws2_32.dll"));
	HMODULE hDnsApi = pApi->LoadLibraryA(STATIC_STR("dnsapi.dll"));


	set(pApi, hKernel32, FreeLibrary);

	set(pApi, hKernel32, GetLastError);
	set(pApi, hKernel32, HeapAlloc);
	set(pApi, hKernel32, HeapFree);
	set(pApi, hKernel32, MultiByteToWideChar);
	set(pApi, hKernel32, WideCharToMultiByte);
	set(pApi, hKernel32, HeapCreate);
	set(pApi, hKernel32, CreateEventA);
	set(pApi, hKernel32, CreateMutexA);
	set(pApi, hKernel32, ReleaseMutex);
	set(pApi, hKernel32, CreateThread);
	set(pApi, hKernel32, TerminateThread);
	set(pApi, hKernel32, lstrcpyA);
	set(pApi, hKernel32, lstrcpyW);
	set(pApi, hKernel32, lstrcatA);
	set(pApi, hKernel32, lstrcatW);
	set(pApi, hKernel32, lstrcmpA);
	set(pApi, hKernel32, lstrcmpW);
	set(pApi, hKernel32, ReleaseMutex);
	set(pApi, hKernel32, CreateFileA);
	set(pApi, hKernel32, WriteFile);
	set(pApi, hKernel32, CloseHandle);
	set(pApi, hKernel32, WaitForSingleObject);
	set(pApi, hKernel32, WaitForMultipleObjects);
	set(pApi, hKernel32, GetModuleFileNameW);
	set(pApi, hKernel32, DeleteFileW);

	set(pApi, hWs2_32, WSASocketA);
	set(pApi, hWs2_32, WSAConnect);
	set(pApi, hWs2_32, WSASend);
	set(pApi, hWs2_32, WSARecv);
	set(pApi, hWs2_32, select);
	set(pApi, hWs2_32, WSAGetOverlappedResult);
	set(pApi, hWs2_32, WSAStartup);
	set(pApi, hWs2_32, WSACleanup);
	set(pApi, hWs2_32, htonl);
	set(pApi, hWs2_32, htons);
	set(pApi, hWs2_32, closesocket);

	set(pApi, hDnsApi, DnsQuery_A);
	
	iRetVal = 0;
out:
	/*
	if (pApi->FreeLibrary) {
		pApi->FreeLibrary(hKernel32);
		pApi->FreeLibrary(hWs2_32);
		pApi->FreeLibrary(hDnsApi);
	}
	*/
	return iRetVal;
}
