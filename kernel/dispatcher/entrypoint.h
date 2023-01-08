#ifndef DISPATCHER_ENTRYPOINT_H
#define DISPATCHER_ENTRYPOINT_H

#include <windef.h>
#include <winsock2.h>

typedef int WINAPI (*LPDISPATCHER_ENTRYPOINT_PROC)(LPVOID pCodeBase, IN_ADDR inaddr, HMODULE WINAPI (*fnLoadLibraryA)(LPCSTR),
		FARPROC WINAPI (*fnGetProcAddress)(HMODULE, LPCSTR));
#endif
