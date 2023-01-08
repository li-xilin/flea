#include "../protocol.h"
#include "kernel/flea.h"
#include "kernel/list.h"
#include "protocol/common.h"

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

INT CmdListFile(LPFLEA fl, SOCKET sock)
{
	typedef struct _FILE_ENTRY
	{
		LINK list;
		DWORD dwSize;
		BOOL bIsDir;
		LPSTR pFileName;
	} FILE_ENTRY, *LPFILE_ENTRY;


	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	INT iRetVal = 0;

	WORD wPathLen;
	if (FleaSyncRecv(fl, sock, &wPathLen, sizeof wPathLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szPath[FS_MAX_PATH];
	if (FleaSyncRecv(fl, sock, &szPath, wPathLen)) {
		iRetVal = -1;
		goto out;
	}

	WCHAR szPathFind[FS_MAX_PATH + 10];;
	LPWSTR pPathUtf16 = FleaConvUtf8ToUtf16(fl, szPath);
	lstrcpyW(szPathFind, pPathUtf16);
	lstrcatW(szPathFind, L"/*");

	hFind = FindFirstFileW(szPathFind, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) {
		struct msg_ret_code ret = { MSG_EOK };
		ret.error_code = GetLastError();
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))
			iRetVal = -1;
		goto out;
	} else {
		struct msg_ret_code ret = { MSG_EOK };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) {
			iRetVal = -1;
			goto out;
		}
	}

	LINK listFiles = LIST_INITIALIZER(listFiles);
	DWORD nFileCount = 0;
	do {
		LPSTR pNameUtf8 = FleaConvUtf16ToUtf8(fl, ffd.cFileName);
		if (!pNameUtf8)
			continue;
		LPFILE_ENTRY entry = HeapAlloc(fl->hHeap, 0, sizeof(FILE_ENTRY));
		entry->dwSize = ffd.nFileSizeLow;
		entry->pFileName = pNameUtf8;
		entry->bIsDir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		ListInitLink(&entry->list);
		ListAddBack(&listFiles, &entry->list);
		nFileCount ++;

	} while (FindNextFileW(hFind, &ffd) != 0);

	if (FleaSyncSend(fl, sock, &nFileCount, sizeof nFileCount)) {
		iRetVal = -1;
		goto out;
	}

	while (!ListIsEmpty(&listFiles)) {

		LPFILE_ENTRY pEntry = ListFirstEntry(&listFiles, FILE_ENTRY, list);
		ListDel(&pEntry->list);

		struct fs_msg_file_entry entry;
		entry.file_size = pEntry->dwSize;
		entry.is_dir = pEntry->bIsDir;
		entry.name_size = lstrlenA(pEntry->pFileName) + 1;

		if (FleaSyncSend(fl, sock, &entry, sizeof entry)) {
			iRetVal = -1;
			goto out;
		}

		if (FleaSyncSend(fl, sock, pEntry->pFileName, entry.name_size)) {
			iRetVal = -1;
			goto out;
		}

		HeapFree(fl->hHeap, 0, pEntry->pFileName);
		HeapFree(fl->hHeap, 0, pEntry);
	}

out:
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);	
	return iRetVal;
}

INT CmdChangeDir(LPFLEA fl, SOCKET sock)
{
	INT iRetVal = 0;
	LPWSTR pPathUtf16 = NULL;

	WORD wPathLen;
	if (FleaSyncRecv(fl, sock, &wPathLen, sizeof wPathLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szPath[FS_MAX_PATH];
	if (FleaSyncRecv(fl, sock, &szPath, wPathLen)) {
		iRetVal = -1;
		goto out;
	}

	pPathUtf16 = FleaConvUtf8ToUtf16(fl, szPath);

	if (!SetCurrentDirectoryW(pPathUtf16)) {
		struct msg_ret_code ret = { GetLastError() };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))
			iRetVal = -1;
		goto out;
	} else {
		struct msg_ret_code ret = { MSG_EOK };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) {
			iRetVal = -1;
			goto out;
		}
	}

out:
	if (pPathUtf16)
		HeapFree(fl->hHeap, 0, pPathUtf16);
	return iRetVal;
}

INT CmdPutFile(LPFLEA fl, SOCKET sock)
{
	INT iRetVal = 0;
	LPWSTR pNameUtf16 = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	struct msg_ret_code ret = { MSG_EOK };

	WORD wNameLen;
	if (FleaSyncRecv(fl, sock, &wNameLen, sizeof wNameLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szName[0x100];
	if (FleaSyncRecv(fl, sock, &szName, wNameLen)) {
		iRetVal = -1;
		goto out;
	}

	pNameUtf16 = FleaConvUtf8ToUtf16(fl, szName);

	hFile = CreateFileW(pNameUtf16, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ret.error_code = GetLastError();
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) 
			iRetVal = -1;
		goto out;
	} else {
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))  {
			iRetVal = -1;
			goto out;
		}
	}

	DWORD dwFileSize;
	if (FleaSyncRecv(fl, sock, &dwFileSize, sizeof dwFileSize)) {
		iRetVal = -1;
		goto out;
	}

	DWORD dwWriten = 0;
	while (dwWriten != dwFileSize) {
		BYTE baBuf[4096];
		DWORD dwToWrite = sizeof baBuf;
		if (dwFileSize - dwWriten < sizeof baBuf)
			dwToWrite = dwFileSize - dwWriten;
		if (FleaSyncRecv(fl, sock, baBuf, dwToWrite)) {
			iRetVal = -1;
			goto out;
		}
		DWORD dwRealWriteSize;
		if (!WriteFile(hFile, baBuf, dwToWrite, &dwRealWriteSize, NULL)) {
			// TODO
		}
		dwWriten += dwToWrite;
	}

	if (FleaSyncSend(fl, sock, &ret, sizeof ret))  {
		iRetVal = -1;
		goto out;
	}
	

out:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		if (iRetVal != 0)
			DeleteFileW(pNameUtf16);
	}
	if (pNameUtf16)
		HeapFree(fl->hHeap, 0, pNameUtf16);
	return iRetVal;
}

INT CmdGetFile(LPFLEA fl, SOCKET sock)
{
	INT iRetVal = 0;
	LPWSTR pNameUtf16 = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	struct msg_ret_code ret = { MSG_EOK };

	WORD wFileNameLen;
	if (FleaSyncRecv(fl, sock, &wFileNameLen, sizeof wFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szFileName[FS_MAX_PATH];
	if (FleaSyncRecv(fl, sock, &szFileName, wFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	pNameUtf16 = FleaConvUtf8ToUtf16(fl, szFileName);

	hFile = CreateFileW(pNameUtf16, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ret.error_code = GetLastError();
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) 
			iRetVal = -1;
		goto out;
	} else {
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))  {
			iRetVal = -1;
			goto out;
		}
	}

	DWORD dwFileSize = GetFileSize(hFile, &dwFileSize);

	if (FleaSyncSend(fl, sock, &dwFileSize, sizeof dwFileSize)) {
		iRetVal = -1;
		goto out;
	}

	WORD dwAlreadyRead = 0;
	while (dwAlreadyRead != dwFileSize) {
		BYTE baBuf[256];
		DWORD dwRealRead;
		if (!ReadFile(hFile, baBuf, sizeof baBuf, &dwRealRead, NULL)) {
			ret.error_code = GetLastError();
			break;
		}
		if (!dwRealRead)
			break;

		WORD wPartSize = dwRealRead;
	
		if (FleaSyncSend(fl, sock, &wPartSize, sizeof wPartSize)) {
			iRetVal = -1;
			goto out;
		}
		
		if (FleaSyncSend(fl, sock, baBuf, dwRealRead)) {
			iRetVal = -1;
			goto out;
		}
		dwAlreadyRead += dwRealRead;

		if (FleaSockWaitRecv(fl, sock, 0) == 0)
			break;

	}

	WORD wPartSize = 0;
	if (FleaSyncSend(fl, sock, &wPartSize, sizeof wPartSize))  {
		iRetVal = -1;
		goto out;
	}

	BYTE byCancel;
	if (FleaSyncRecv(fl, sock, &byCancel, sizeof byCancel)) {
		iRetVal = -1;
		goto out;
	}
	
	if (FleaSyncSend(fl, sock, &ret, sizeof ret))  {
		iRetVal = -1;
		goto out;
	}
out:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	if (pNameUtf16)
		HeapFree(fl->hHeap, 0, pNameUtf16);
	return iRetVal;
}


INT CmdGetCurrentDir(LPFLEA fl, SOCKET sock)
{
	INT iRetVal = 0;
	struct msg_ret_code ret = { MSG_EOK };
	LPSTR pCurrentDirUtf8 = NULL;

	WCHAR szCurrentDir[PATH_MAX];
	DWORD dwDirLength = GetCurrentDirectoryW(sizeof szCurrentDir, szCurrentDir);
	if (dwDirLength == 0) {
		ret.error_code = GetLastError();
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))
			iRetVal = -1;
		goto out;
	} else {
		szCurrentDir[dwDirLength] = '\0';
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) {
			iRetVal = -1;
			goto out;
		}
	}

	pCurrentDirUtf8 = FleaConvUtf16ToUtf8(fl, szCurrentDir);

	WORD dwDirUtf8Len = lstrlenA(pCurrentDirUtf8) + 1;

	if (FleaSyncSend(fl, sock, &dwDirUtf8Len, sizeof dwDirUtf8Len)) {
		iRetVal = -1;
		goto out;
	}

	if (FleaSyncSend(fl, sock, pCurrentDirUtf8, dwDirUtf8Len)) {
		iRetVal = -1;
		goto out;
	}
out:
	if (pCurrentDirUtf8)
		HeapFree(fl->hHeap, 0, pCurrentDirUtf8);

	return iRetVal;
}

INT CmdDeleteFile(LPFLEA fl, SOCKET sock)
{
	INT iRetVal = 0;
	LPWSTR pFileNameUtf16 = NULL;

	WORD wFileNameLen;
	if (FleaSyncRecv(fl, sock, &wFileNameLen, sizeof wFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szFileName[FS_MAX_PATH];
	if (FleaSyncRecv(fl, sock, &szFileName, wFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	pFileNameUtf16 = FleaConvUtf8ToUtf16(fl, szFileName);

	if (!DeleteFileW(pFileNameUtf16)) {
		struct msg_ret_code ret = { GetLastError() };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))
			iRetVal = -1;
		goto out;
	} else {
		struct msg_ret_code ret = { MSG_EOK };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) {
			iRetVal = -1;
			goto out;
		}
	}

out:
	if (pFileNameUtf16)
		HeapFree(fl->hHeap, 0, pFileNameUtf16);
	return iRetVal;
}

INT CmdMoveFile(LPFLEA fl, SOCKET sock)
{
	INT iRetVal = 0;
	LPWSTR pOldFileNameUtf16 = NULL;
	LPWSTR pNewFileNameUtf16 = NULL;

	WORD wOldFileNameLen;
	if (FleaSyncRecv(fl, sock, &wOldFileNameLen, sizeof wOldFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szOldFileName[FS_MAX_PATH];
	if (FleaSyncRecv(fl, sock, &szOldFileName, wOldFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	pOldFileNameUtf16 = FleaConvUtf8ToUtf16(fl, szOldFileName);


	WORD wNewFileNameLen;
	if (FleaSyncRecv(fl, sock, &wNewFileNameLen, sizeof wNewFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	CHAR szNewFileName[FS_MAX_PATH];
	if (FleaSyncRecv(fl, sock, &szNewFileName, wNewFileNameLen)) {
		iRetVal = -1;
		goto out;
	}

	pNewFileNameUtf16 = FleaConvUtf8ToUtf16(fl, szNewFileName);


	if (!MoveFileExW(pOldFileNameUtf16, pNewFileNameUtf16, MOVEFILE_COPY_ALLOWED
			| MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		struct msg_ret_code ret = { GetLastError() };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret))
			iRetVal = -1;
		goto out;
	} else {
		struct msg_ret_code ret = { MSG_EOK };
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) {
			iRetVal = -1;
			goto out;
		}
	}

out:
	if (pOldFileNameUtf16)
		HeapFree(fl->hHeap, 0, pOldFileNameUtf16);
	if (pNewFileNameUtf16)
		HeapFree(fl->hHeap, 0, pNewFileNameUtf16);
	return iRetVal;
}

INT Initialize(LPFLEA fl, INT iModIndex)
{
	PCALLTABLE ct = &fl->call;
	int index = ct->TunnelBind(fl, iModIndex, TUNNEL);
	if (index == -1)
		return -1;
	SOCKET sock = FleaGetTunnelSocket(fl, index);
	BYTE byID;

	while (1) {
		if (FleaSyncRecv(fl, sock, &byID, sizeof byID))
			return -1;
		switch (byID) {
			case FS_MSG_LIST:
				if (CmdListFile(fl, sock))
					return -1;
				break;
			case FS_MSG_PWD:
				if (CmdGetCurrentDir(fl, sock))
					return -1;
				break;
			case FS_MSG_CHDIR:
				if (CmdChangeDir(fl, sock))
					return -1;
				break;
			case FS_MSG_MOVE:
				if (CmdMoveFile(fl, sock))
					return -1;
				break;
			case FS_MSG_DELETE:
				if (CmdDeleteFile(fl, sock))
					return -1;
				break;
			case FS_MSG_PUT:
				if (CmdPutFile(fl, sock))
					return -1;
				break;
			case FS_MSG_GET:
				if (CmdGetFile(fl, sock))
					return -1;
				break;
		}
	}
	
	FleaTunnelClose(fl, index);
	return 0;
}

void Interrupt(LPFLEA d, INT iModIndex)
{
}
