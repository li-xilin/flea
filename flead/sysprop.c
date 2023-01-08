#include "sysprop.h"
#include <stdio.h>
#include <ioapiset.h>
#include <windows.h>

static int GetDiskSerialNumber(LPSTR pBuffer, DWORD dwSize);

int WriteToLog(char* str);

int GetSysProp(LPSYSPROP pSysProp)
{
	ZeroMemory(pSysProp, sizeof(SYSPROP));

        DWORD dwNameSize = sizeof pSysProp->szHostName;
        GetComputerNameA(pSysProp->szHostName, &dwNameSize);
	pSysProp->szHostName[dwNameSize] = '\0';

	if (GetDiskSerialNumber(pSysProp->szDiskSerialName, sizeof pSysProp->szDiskSerialName))
		return -1;


	pSysProp->bInWOW64 = FALSE;
	BOOL WINAPI (*IsWow64Process) (HANDLE, PBOOL) =
			GetProcAddress(GetModuleHandleW((L"kernel32")), "IsWow64Process");

        if (IsWow64Process)
                IsWow64Process(GetCurrentProcess(),&pSysProp->bInWOW64);

        OSVERSIONINFOEX version = { sizeof(OSVERSIONINFOEX) };
        if(!GetVersionEx((OSVERSIONINFO *)&version)) {
                return 0;
        }

	pSysProp->wMajorVersion = version.dwMajorVersion;
	pSysProp->wMinorVersion = version.dwMinorVersion;
	pSysProp->bWorkstation = version.wProductType == VER_NT_WORKSTATION;

	return 0;
}

static int GetDiskSerialNumber(LPSTR pBuffer, DWORD dwSize)
{
	int iRetVal = -1;
	WCHAR wzDriverName[] = L"\\\\.\\PhysicalDrive0";

        HANDLE hDevice = NULL;
	STORAGE_DEVICE_DESCRIPTOR *pDeviceDesc = NULL;

	hDevice = CreateFileW(wzDriverName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE)
		goto out;

        STORAGE_PROPERTY_QUERY query = { 
		.PropertyId = StorageDeviceProperty,
		.QueryType = PropertyStandardQuery,
	};

        DWORD dwBytesReturned = 0;
        STORAGE_DESCRIPTOR_HEADER DeviceHeader;
        if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof query,
			&DeviceHeader, sizeof DeviceHeader, &dwBytesReturned, NULL))
		goto out;

        pDeviceDesc = HeapAlloc(GetProcessHeap(), 0, DeviceHeader.Size);
	if (!pDeviceDesc)
		goto out;

        if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof query, pDeviceDesc, DeviceHeader.Size, NULL, NULL))
		goto out;

        LPCSTR pSerialNumber = (LPCSTR)pDeviceDesc + pDeviceDesc->SerialNumberOffset;

	DWORD dwNumberSize = lstrlenA(pSerialNumber) + 1;
	if (dwSize > dwNumberSize)
		dwSize = dwNumberSize;

	CopyMemory(pBuffer, pSerialNumber, dwSize);

	iRetVal = 0;
out:

        if (hDevice == INVALID_HANDLE_VALUE)
		CloseHandle(hDevice);
	if (pDeviceDesc)
		HeapFree(GetProcessHeap(), 0, pDeviceDesc);
	return iRetVal;
}

