#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>

int RunServer(void *pCodeBase, IN_ADDR inaddr, HMODULE WINAPI (*fnLoadLibraryA)(LPCSTR),
		FARPROC WINAPI (*fnGetProcAddress)(HMODULE, LPCSTR));
#endif
