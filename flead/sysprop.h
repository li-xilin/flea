#ifndef SYSPROP_H
#define SYSPROP_H

#include <windef.h>
#include <winbase.h>

typedef struct {
	CHAR szHostName[MAX_COMPUTERNAME_LENGTH + 1];
	CHAR szDiskSerialName[0xFF];
	WORD wMajorVersion;
	WORD wMinorVersion;
	BOOL bInWOW64;
	BOOL bWorkstation;
} SYSPROP, *LPSYSPROP;

int GetSysProp(LPSYSPROP pSysProp);

#endif
