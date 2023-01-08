#include "server.h"
#include "config.h"
#include <windef.h>
#include "entrypoint.h"
#include <winsock2.h>

#ifdef DEBUG_BUILD

#include <windns.h>
#include <stdio.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	PDNS_RECORD pDnsRecord;
	if (DnsQuery_A(CONFIG_RELAY_HOST, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL)) {
		fprintf(stderr, "DnsQuery_A: error %ld", GetLastError());
		return 1;
	}

	IN_ADDR inaddr;
	inaddr.S_un.S_addr = pDnsRecord->Data.A.IpAddress;
	return RunServer(NULL, inaddr, LoadLibraryA, GetProcAddress);
}

#else

int WINAPI start(LPVOID pBase, IN_ADDR inaddr, HMODULE WINAPI (*fnLoadLibraryA)(LPCSTR),
                FARPROC WINAPI (*fnGetProcAddress)(HMODULE, LPCSTR))
{
	return RunServer(pBase, inaddr, fnLoadLibraryA, fnGetProcAddress);
}

LPDISPATCHER_ENTRYPOINT_PROC pProc = start; // Verify function signiture

#endif
