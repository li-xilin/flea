#ifndef SYS_H
#define SYS_H
#include "dld.h"
#include <winsock2.h>
typedef struct {
	HMODULE WINAPI (*pfnLoadLibraryA)(LPCSTR lpFileName);

	FARPROC WINAPI (*pfnGetProcAddress)(HMODULE hModule, LPCSTR  lpProcName);

	int WSAAPI (*pfnWSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);

	SOCKET WSAAPI (*pfnWSASocketA)(int af, int type, int protocol,
			LPWSAPROTOCOL_INFOA lpProtocolInfo, GROUP g, DWORD dwFlags); 

	int WSAAPI (*pfnWSAConnect)(SOCKET s, const struct sockaddr *name, int  namelen,
			LPWSABUF  lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS); 

	int (WSAAPI *pfnWSARecv)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
			LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped,
			LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

	// int (*pfnWSAGetLastError)(void);

	LPVOID WINAPI (*pfnVirtualAlloc)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
} SYSTAB, *LPSYSTAB;

DWORD InitSysTab(LPSYSTAB pSysTable);

#endif

