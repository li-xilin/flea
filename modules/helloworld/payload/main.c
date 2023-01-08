#include "kernel/flea.h"
#include <winsock2.h>

#define TUNNEL 5

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch( fdwReason ) { 
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

INT Initialize(LPFLEA fl, INT iModIndex)
{
	PCALLTABLE ct = &fl->call;
	int index = ct->TunnelBind(fl, iModIndex, TUNNEL);
	if (index == -1)
		return -1;
	SOCKET sock = FleaGetTunnelSocket(fl, index);
	CHAR szText[] = "Hello world";
	if (FleaSyncSend(fl, sock, szText, sizeof szText))
		return -1;
	
	fl->call.TunnelClose(fl, index);
	return 0;
}

void Interrupt(LPFLEA d, INT iModIndex)
{
}
