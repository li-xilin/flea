#ifndef KERNEL_FLEA_H
#define KERNEL_FLEA_H

#include "api.h"

#include <windef.h>
#include <winsock2.h>

#define MOD_MAX 64
#define TUN_MAX 256
#define MOD_NAME_MAX 16

#define STATIC_STR(str) ((CHAR[]) { str })
#define STATIC_WSTR(wstr) ((WCHAR[]) { wstr })

typedef struct _FLEA FLEA, *LPFLEA;

typedef INT (*PFNINITIALIZE_PROC)(LPFLEA d, INT iModIndex);
typedef VOID (*PFNINTERRUPT_PROC)(LPFLEA d, INT iModIndex);

typedef struct
{
	struct _MOD_ITEM {
		CHAR szName[MOD_NAME_MAX];
		HMODULE hModule;
		HANDLE hThread;
		LPVOID pPrivate;
		PFNINTERRUPT_PROC pfnStopProc;
		enum MOD_STATUS {
			MOD_STATUS_NONE,
			MOD_STATUS_ACTIVE,
			MOD_STATUS_EXITED,
		} status;
	} aModule[MOD_MAX];
	SIZE_T nModuleCount;
	BOOL bChanged;
} MOD, *PMOD;

typedef struct _MOD_ITEM MOD_ITEM, *PMOD_ITEM;

typedef struct
{
	struct {
		SOCKET sock;
		INT iModIndex;
	} aTunnel[TUN_MAX];
	UINT nTunnelCount;
	SOCKADDR_IN addr;
	DWORD dwToken;
	HANDLE hMutex;
} TUN, *PTUN;

typedef struct
{
	LPFLEA pData;
	INT iModIndex;
	PFNINITIALIZE_PROC pfnModuleEntry;
} MOD_THREAD_PARAM, *PMOD_THREAD_PARAM;

typedef struct
{
	INT (*TunnelBind)(LPFLEA fl, DWORD dwModIndex, UINT nTunnel);
#define FleaTunnelBind(fl, dwModIndex, nTunnel) fl->call.TunnelBind(fl, dwModIndex, nTunnel)
	VOID (*TunnelClose)(LPFLEA fl, DWORD dwIndex);
#define FleaTunnelClose(fl, dwIndex) fl->call.TunnelClose(fl, dwIndex)
	INT (*SyncRecv)(LPFLEA fl, SOCKET sock, LPVOID pBuffer, DWORD dwSize);
#define FleaSyncRecv(fl, sock, pBuffer, dwSize) fl->call.SyncRecv(fl, sock, pBuffer, dwSize)
	INT (*SyncSend)(LPFLEA fl, SOCKET sock, LPVOID pData, DWORD dwSize);
#define FleaSyncSend(fl, sock, pData, dwSize) fl->call.SyncSend(fl, sock, pData, dwSize)
	INT (*SockWaitRecv)(LPFLEA fl, SOCKET sock, DWORD dwMillise);
#define FleaSockWaitRecv(fl, sock, dwMillise) fl->call.SockWaitRecv(fl, sock, dwMillise)
	LPWSTR (*ConvUtf8ToUtf16)(LPFLEA fl, LPCSTR pszStr);
#define FleaConvUtf8ToUtf16(fl, pszStr) fl->call.ConvUtf8ToUtf16(fl, pszStr)
	LPSTR (*ConvUtf16ToUtf8)(LPFLEA fl, LPWSTR pwzStr);
#define FleaConvUtf16ToUtf8(fl, pwzStr) fl->call.ConvUtf16ToUtf8(fl, pwzStr)
} CALLTABLE, *PCALLTABLE;

struct _FLEA
{
	MOD mod;
	TUN tun;
	CALLTABLE call;
	WIN32API api;
	SOCKET sock;
	HANDLE hLogEvent;
	HANDLE hHeap;
	LPVOID pBase;
};

inline static SOCKET FleaGetTunnelSocket(LPFLEA d, UINT nTunnel)
{
	return d->tun.aTunnel[nTunnel].sock;
}

#endif
