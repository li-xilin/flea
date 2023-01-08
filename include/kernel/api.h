#ifndef KERNEL_API_H
#define KERNEL_API_H
#include <windef.h>
#include <windns.h>
#include <winsock2.h>

typedef struct
{
	HMODULE WINAPI (*LoadLibraryA)(LPCSTR lpLibFileName);
	WINBOOL WINAPI (*FreeLibrary) (HMODULE hLibModule);
	FARPROC WINAPI (*GetProcAddress)( HMODULE hModule, LPCSTR lpProcName);


	DNS_STATUS WINAPI (*DnsQuery_A)( //dnsapi
			PCSTR       pszName,
			WORD        wType,
			DWORD       Options,
			PVOID       pExtra,
			PDNS_RECORD *ppQueryResults,
			PVOID       *pReserved
			);

	DWORD (*GetLastError)();

	int WINAPI (*MultiByteToWideChar)( //kernel32
			UINT CodePage,
			DWORD dwFlags,
			LPCCH lpMultiByteStr,
			int cbMultiByte,
			LPWSTR lpWideCharStr,
			int cchWideChar
			);

	LPVOID WINAPI (*HeapAlloc)(
			HANDLE hHeap,
			DWORD dwFlags,
			SIZE_T dwBytes
			);

	int WINAPI (*WideCharToMultiByte)( //kernel32
			UINT CodePage,
			DWORD dwFlags,
			LPCWCH lpWideCharStr,
			int cchWideChar,
			LPSTR lpMultiByteStr,
			int cbMultiByte,
			LPCCH lpDefaultChar,
			LPBOOL lpUsedDefaultChar
			);

	WINBOOL WINAPI (*HeapFree)( //kernel32
			HANDLE hHeap,
			DWORD dwFlags,
			LPVOID lpMem
			);

	HANDLE WINAPI (*HeapCreate)( //kernel32
			DWORD flOptions,
			SIZE_T dwInitialSize,
			SIZE_T dwMaximumSize
			);

	HANDLE WINAPI (*CreateEventA)( //kernel32
			LPSECURITY_ATTRIBUTES lpEventAttributes,
			WINBOOL bManualReset,
			WINBOOL bInitialState,
			LPCSTR lpName
			);

	HANDLE WINAPI (*CreateMutexA)( //kernel32k
			LPSECURITY_ATTRIBUTES lpMutexAttributes,
			WINBOOL bInitialOwner,
			LPCSTR lpName
			);

	HANDLE WINAPI (*ReleaseMutex)(HANDLE hMutex);

	HANDLE WINAPI (*CloseHandle)(HANDLE h);

	SOCKET WSAAPI (*WSASocketA)( //ws2_32
			int af,
			int type,
			int protocol,
			LPWSAPROTOCOL_INFOA lpProtocolInfo,
			GROUP g,
			DWORD dwFlags
			);

	int WSAAPI (*WSAConnect)(
			SOCKET s,
			const struct sockaddr *name,
			int namelen,
			LPWSABUF lpCallerData,
			LPWSABUF lpCalleeData,
			LPQOS lpSQOS,
			LPQOS lpGQOS
			);

	int WSAAPI (*WSASend)(
			SOCKET s,
			LPWSABUF lpBuffers,
			DWORD dwBufferCount,
			LPDWORD lpNumberOfBytesSent,
			DWORD dwFlags,
			LPWSAOVERLAPPED lpOverlapped,
			LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
			);

	int WSAAPI (*WSARecv)(
			SOCKET s,
			LPWSABUF lpBuffers,
			DWORD dwBufferCount,
			LPDWORD lpNumberOfBytesRecvd,
			LPDWORD lpFlags,
			LPWSAOVERLAPPED lpOverlapped,
			LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
			);

	u_long WSAAPI (*htonl)(u_long hostlong);

	u_short WSAAPI (*htons)(u_short hostshort);

	DWORD WINAPI (*WaitForSingleObject)(HANDLE hHandle, DWORD dwMilliseconds);

	DWORD WINAPI (*WaitForMultipleObjects)(
			DWORD nCount,
			CONST HANDLE *lpHandles,
			WINBOOL bWaitAll,
			DWORD dwMilliseconds
			);


	int WSAAPI (*select)( //ws2_32
			int nfds,
			fd_set *readfds,
			fd_set *writefds,
			fd_set *exceptfds,
			const PTIMEVAL timeout
			);

	WINBOOL WSAAPI (*WSAGetOverlappedResult)(
			SOCKET s,
			LPWSAOVERLAPPED lpOverlapped,
			LPDWORD lpcbTransfer,
			WINBOOL fWait,
			LPDWORD lpdwFlags
			);

	HANDLE WINAPI (*CreateThread)(
			LPSECURITY_ATTRIBUTES lpThreadAttributes,
			SIZE_T dwStackSize,
			LPTHREAD_START_ROUTINE lpStartAddress,
			LPVOID lpParameter,
			DWORD dwCreationFlags,
			LPDWORD lpThreadId
			);

	int WSAAPI (*closesocket)(SOCKET s);

	WINBOOL WINAPI (*TerminateThread)(HANDLE hThread, DWORD dwExitCode);

	LPSTR WINAPI (*lstrcpyA)(LPSTR lpString1, LPCSTR lpString2);

	LPWSTR WINAPI (*lstrcpyW)(LPWSTR lpString1, LPCWSTR lpString2);

	LPSTR WINAPI (*lstrcatA)(LPSTR lpString1, LPCSTR lpString2);

	LPWSTR WINAPI (*lstrcatW)(LPWSTR lpString1, LPCWSTR lpString2);
	int WINAPI (*lstrcmpA) (LPCSTR lpString1, LPCSTR lpString2);
	int WINAPI (*lstrcmpW) (LPCWSTR lpString1, LPCWSTR lpString2);

	int WSAAPI (*WSAStartup)(WORD wVersionRequested, LPWSADATA lpWSAData);

	int WSAAPI (*WSACleanup)(void);

	HANDLE WINAPI (*CreateFileA)(
			LPCSTR lpFileName,
			DWORD dwDesiredAccess,
			DWORD dwShareMode,
			LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			DWORD dwCreationDisposition,
			DWORD dwFlagsAndAttributes,
			HANDLE hTemplateFile
			);

	WINBOOL WINAPI (*WriteFile)(
			HANDLE hFile,
			LPCVOID lpBuffer,
			DWORD nNumberOfBytesToWrite,
			LPDWORD lpNumberOfBytesWritten,
			LPOVERLAPPED lpOverlapped
			);

	DWORD WINAPI (*GetModuleFileNameW) (HMODULE hModule, LPWSTR lpFilename, DWORD nSize);

	WINBOOL WINAPI (*DeleteFileW) (LPCWSTR lpFileName);
} WIN32API, *LPWIN32API;

INT InitializeWin32Api(LPWIN32API pApi);

#endif
