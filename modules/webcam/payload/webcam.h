#ifndef WEBCAM_H
#define  WEBCAM_H

#include "kernel/list.h"
#include <windef.h>

typedef struct _WEBCAM WEBCAM, *LPWEBCAM;

INT WebcamInitialize();

VOID WebcamUninitialize();

LPWEBCAM WebcamOpen(HANDLE hHeap, DWORD dwIndex);

INT WebcamCaptureJpeg(LPWEBCAM pCam, HGLOBAL hMem);

VOID WebcamClose(LPWEBCAM pCam);


typedef struct _WEBCAM_NAME
{
	LINK link;
	WCHAR wzName[256];
} WEBCAM_NAME, *LPWEBCAM_NAME;

INT WebcamListDevice(HANDLE hHeap, LPLINK pList);

#endif
