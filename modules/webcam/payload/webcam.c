#include "webcam.h"
#define COBJMACROS
#define STRSAFE_NO_DEPRECATE
#include <qedit.h>
#include <dshow.h>
#include <wincodec.h>
#include <combaseapi.h>
#include <amvideo.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#define SIZE_PREHEADER offsetof(VIDEOINFOHEADER, bmiHeader);

EXTERN_C const CLSID CLSID_NullRenderer;
EXTERN_C const CLSID CLSID_SampleGrabber;

struct _WEBCAM {
        HANDLE hHeap;
	ISampleGrabber *pSampleGrabber;
	IBaseFilter *pNullRenderer;
	IMediaControl *pMediaControl;
	AM_MEDIA_TYPE mediaType;
};

static INT RGB24BufferToJpeg(LPBYTE pBuffer, DWORD dwWidth, DWORD dwHeight, HGLOBAL hMem);

INT WebcamInitialize()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
		return -1;
	return 0;
}

VOID WebcamUninitialize()
{
	CoUninitialize();
}

LPWEBCAM WebcamOpen(HANDLE hHeap, DWORD dwIndex)
{

	HRESULT hr;
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	IMoniker *pMoniker = NULL;

	IGraphBuilder *pGraph = NULL;
	ICaptureGraphBuilder2 *pBuilder = NULL;
	IBaseFilter *pCap = NULL;
	IBaseFilter *pSampleGrabberFilter = NULL;
	ISampleGrabber *pSampleGrabber = NULL;
	IBaseFilter *pNullRenderer = NULL;
	IMediaControl *pMediaControl = NULL;

	LPWEBCAM pCam = NULL;

	pCam = HeapAlloc(hHeap, 0, sizeof(WEBCAM));
	if (!pCam)
		goto out;
	pCam->hHeap = hHeap;

	hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID *)&pGraph);
	if (hr != S_OK) {
		puts("Could not create filter graph");
		goto out;
	}
	
	// Create capture graph builder.
	hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, &IID_ICaptureGraphBuilder2, (void **)&pBuilder);
	if (hr != S_OK) {
		puts("Could not create capture graph builder");
		goto out;
	}

	// Attach capture graph builder to graph
	hr = ICaptureGraphBuilder2_SetFiltergraph(pBuilder, pGraph);
	if (hr != S_OK) {
		puts("Could not attach capture graph builder to graph");
		goto out;
	}

	// hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL,CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum), NULL);
	hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL,CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (LPVOID *)&pDevEnum);
	if (hr != S_OK) {
		puts("Could not crerate system device enumerator");
		goto out;
	}

	// Video input device enumerator
	hr = ICreateDevEnum_CreateClassEnumerator(pDevEnum, &CLSID_VideoInputDeviceCategory, &pEnum, 0);
	if (hr != S_OK) {
		puts("No video devices found");
		goto out;
	}
	
	for (int i = 0; i <= dwIndex; i++) {
		hr = IEnumMoniker_Next(pEnum, 1, &pMoniker, NULL);
		if (hr != S_OK) {
			puts("not found");
			goto out;
		}
		if (i != dwIndex)
			IMoniker_Release(pMoniker);
	}
	
	hr = IMoniker_BindToObject(pMoniker, 0, 0, &IID_IBaseFilter, (LPVOID *)&pCap);
	if (hr != S_OK) {
		puts("Could not create capture filter");
		goto out;
	}
		
	hr = IGraphBuilder_AddFilter(pGraph, pCap, L"Capture Filter");
	if (hr != S_OK) {
		puts("Could not add capture filter to graph");
		goto out;
	}

	hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID *)&pSampleGrabberFilter);
	if (hr != S_OK) {
		puts("Could not create Sample Grabber filter");
		goto out;
	}
	
	// Query the ISampleGrabber interface of the sample grabber filter
	hr = IBaseFilter_QueryInterface(pSampleGrabberFilter, &IID_ISampleGrabber, (LPVOID *)&pSampleGrabber);
	if (hr != S_OK) {
		puts("Could not get ISampleGrabber interface to sample grabber filter");
		goto out;
	}

	
	// Enable sample buffering in the sample grabber filter
	hr = ISampleGrabber_SetBufferSamples(pSampleGrabber, TRUE);
	if (hr != S_OK) {
		puts("Could not enable sample buffering in the sample grabber");
		goto out;
	}

	// Set media type in sample grabber filter
	ZeroMemory(&pCam->mediaType, sizeof(AM_MEDIA_TYPE));
	pCam->mediaType.majortype = MEDIATYPE_Video;
	pCam->mediaType.subtype = MEDIASUBTYPE_RGB24;
	hr = ISampleGrabber_SetMediaType(pSampleGrabber, (AM_MEDIA_TYPE *)&pCam->mediaType);
	if (hr != S_OK) {
		puts("Could not set media type in sample grabber");
		goto out;
	}
	
	// Add sample grabber filter to filter graph
	hr = IGraphBuilder_AddFilter(pGraph, pSampleGrabberFilter, L"SampleGrab");
	if (hr != S_OK) {
		puts("Could not add Sample Grabber to filter graph");
		goto out;
	}

	// Create Null Renderer filter
	hr = CoCreateInstance(&CLSID_NullRenderer, NULL,
		CLSCTX_INPROC_SERVER, &IID_IBaseFilter,
		(void**)&pNullRenderer);
	if (hr != S_OK) {
		puts("Could not create Null Renderer filter");
		goto out;
	}
	
	// Add Null Renderer filter to filter graph
	hr = IGraphBuilder_AddFilter(pGraph, pNullRenderer, L"NullRender");
	if (hr != S_OK) {
		puts("Could not add Null Renderer to filter graph");
		goto out;
	}
	
	// Connect up the filter graph's capture stream
	hr = ICaptureGraphBuilder2_RenderStream(pBuilder, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, (IUnknown *)pCap,  pSampleGrabberFilter, pNullRenderer);
	if (hr != S_OK) {
		puts("Could not render capture video stream");
		goto out;
	}
		
	// Get media control interfaces to graph builder object
	hr = IGraphBuilder_QueryInterface(pGraph, &IID_IMediaControl, (void**)&pMediaControl);
	if (hr != S_OK) {
		puts("Could not get media control interface");
		goto out;
	}
	
	hr = S_FALSE;
	while (hr != S_OK) {
		hr = IMediaControl_Run(pMediaControl);
	}

	// Get the media type from the sample grabber filter
	hr = ISampleGrabber_GetConnectedMediaType(pSampleGrabber, (AM_MEDIA_TYPE *)&pCam->mediaType);
	if (hr != S_OK) {
		puts("Could not get media type");
		goto out;
	}
	
	if ( !IsEqualGUID(&pCam->mediaType.formattype, &FORMAT_VideoInfo)
			|| (pCam->mediaType.cbFormat < sizeof(VIDEOINFOHEADER))
			|| (pCam->mediaType.pbFormat == NULL)) {
		puts("Wrong media type");
		goto out;
	}

	pCam->pSampleGrabber = pSampleGrabber;
	pCam->pNullRenderer = pNullRenderer;
	pCam->pMediaControl = pMediaControl;

	IMoniker_Release(pMoniker);
	IEnumMoniker_Release(pEnum);
	ICreateDevEnum_Release(pDevEnum);
	IGraphBuilder_Release(pGraph);
	IBaseFilter_Release(pSampleGrabberFilter);
	IBaseFilter_Release(pCap);
	ICaptureGraphBuilder2_Release(pBuilder);


	return pCam;
out:
	if (pMediaControl)
		IMediaControl_Release(pMediaControl);	
	if (pNullRenderer) 
		IBaseFilter_Release(pNullRenderer);
	if (pSampleGrabber)
		ISampleGrabber_Release(pSampleGrabber);
	if (pSampleGrabberFilter)
		IBaseFilter_Release(pSampleGrabberFilter);
	if (pCap)
		IBaseFilter_Release(pCap);
	if (pBuilder)
		ICaptureGraphBuilder2_Release(pBuilder);
	if (pGraph)
		IGraphBuilder_Release(pGraph);
	if (pMoniker)
		IMoniker_Release(pMoniker);
	if (pEnum)
		IEnumMoniker_Release(pEnum);
	if (pDevEnum)
		ICreateDevEnum_Release(pDevEnum);

	return NULL;
}

INT WebcamCaptureJpeg(LPWEBCAM pCam, HGLOBAL hMem)
{
	INT iRetVal = -1;
	HRESULT hr;
	LPBYTE pBuffer = NULL;

	LONG iBufferSize = 0;
	while(1) {
		// checking the required buffer size
		hr = ISampleGrabber_GetCurrentBuffer(pCam->pSampleGrabber, &iBufferSize, NULL);
		
		// Keep trying until buffer_size is set to non-zero value.
		if (hr == S_OK && iBufferSize != 0)
			break;
		// something has gone wrong
		if (hr != S_OK &&hr != VFW_E_WRONG_STATE) {
			puts("Could not get buffer size");
			goto out;
		}
	}

	pBuffer = HeapAlloc(pCam->hHeap, 0, iBufferSize);
	if (!pBuffer) {
		puts("HeapAlloc");
		goto out;
	}
	
	// Retrieve image data from sample grabber buffer
	hr = ISampleGrabber_GetCurrentBuffer(pCam->pSampleGrabber, &iBufferSize, (long*)pBuffer);
	if (hr != S_OK) {
		puts("Could not get buffer data from sample grabber");
		goto out;
	}

	VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pCam->mediaType.pbFormat;

	if (RGB24BufferToJpeg(pBuffer, pVih->bmiHeader.biWidth, pVih->bmiHeader.biHeight, hMem))
		goto out;
	iRetVal = 0;
out:

	if (pBuffer)
		HeapFree(pCam->hHeap, 0, pBuffer);;
	return iRetVal;

	
}

VOID WebcamClose(LPWEBCAM pCam)
{

	IMediaControl_Stop(pCam->pMediaControl);

	// Free the format block
	if (pCam->mediaType.cbFormat) {
		CoTaskMemFree((PVOID)pCam->mediaType.pbFormat);
		pCam->mediaType.cbFormat = 0;
		pCam->mediaType.pbFormat = NULL;
	}

	if (pCam->mediaType.pUnk) {
		IUnknown_Release(pCam->mediaType.pUnk);
		pCam->mediaType.pUnk = NULL;
	}

	IMediaControl_Release(pCam->pMediaControl);	
	IBaseFilter_Release(pCam->pNullRenderer);
	ISampleGrabber_Release(pCam->pSampleGrabber);

	HeapFree(pCam->hHeap, 0, pCam);

}

INT WebcamListDevice(HANDLE hHeap, LPLINK pList)
{
	HRESULT hr;
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;

	ListInit(pList);

	hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL,CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (LPVOID *)&pDevEnum);
	if (hr != S_OK)
		goto fail;

	hr = ICreateDevEnum_CreateClassEnumerator(pDevEnum, &CLSID_VideoInputDeviceCategory, &pEnum, 0);
	if (hr != S_OK)
		goto fail;

	IMoniker *pMoniker = NULL;
	for (int i = 0; (IEnumMoniker_Next(pEnum, 1, &pMoniker, NULL)) == S_OK; i++) {

		IPropertyBag *pPropBag = NULL;
		hr = IMoniker_BindToStorage(pMoniker, 0, 0, &IID_IPropertyBag, (LPVOID *)&pPropBag);

		VARIANT var;
		VariantInit(&var);

		hr = IPropertyBag_Read(pPropBag, L"FriendlyName", &var, 0);

		LPWEBCAM_NAME pNameNode = HeapAlloc(hHeap, 0, sizeof(WEBCAM_NAME));
		lstrcpyW(pNameNode->wzName, var.bstrVal);


		ListAddBack(pList, &pNameNode->link);
		
		VariantClear(&var);

		IMoniker_Release(pMoniker);
	}

	IEnumMoniker_Release(pEnum);
	ICreateDevEnum_Release(pDevEnum);
	return 0;
fail:
	if (pEnum != NULL)
		IEnumMoniker_Release(pEnum);
	if (pDevEnum != NULL)
		ICreateDevEnum_Release(pDevEnum);
	
	while (!ListIsEmpty(pList)) {
		LPWEBCAM_NAME pCamName = ListFirstEntry(pList, WEBCAM_NAME, link);;
		ListDel(&pCamName->link);
		HeapFree(hHeap, 0, pCamName);
	}

	return -1;
}

INT RGB24BufferToJpeg(LPBYTE pBuffer, DWORD dwWidth, DWORD dwHeight, HGLOBAL hMem)
{
	INT iRetVal = -1;
        IWICImagingFactory* piFactory = NULL;
        IWICBitmapEncoder* piEncoder = NULL;
        IWICBitmapFrameEncode* piBitmapFrame = NULL;
        IWICBitmap* piBitmapSrc = NULL;
        IWICFormatConverter* piFormatConverter = NULL;
        IWICBitmapSource* piBitmapTmp = NULL;
	IStream *piStream = NULL;

        HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (LPVOID*)&piFactory);
        if (FAILED(hr))
		goto out;

	hr = CreateStreamOnHGlobal(hMem, FALSE, &piStream);
        if (FAILED(hr))
		goto out;

	hr = IWICImagingFactory_CreateEncoder(piFactory, &GUID_ContainerFormatJpeg, NULL, &piEncoder);
        if (FAILED(hr))
		goto out;

	hr = IWICBitmapEncoder_Initialize(piEncoder, (IStream *)piStream, WICBitmapEncoderNoCache);
        if (FAILED(hr))
		goto out;


	IPropertyBag2* pPropertybag = NULL;
	hr = IWICBitmapEncoder_CreateNewFrame(piEncoder, &piBitmapFrame, &pPropertybag);
	if (FAILED(hr))
		goto out;

	PROPBAG2 option = {0};
	option.pstrName = (LPOLESTR)L"BitmapTransform";
	VARIANT varValue;
	VariantInit(&varValue);
	varValue.vt = VT_UI1;
	varValue.bVal = WICBitmapTransformFlipVertical;
	hr = IPropertyBag2_Write(pPropertybag, 1, &option, &varValue);
	if (FAILED(hr)) {
		IPropertyBag2_Release(pPropertybag);
		goto out;
	}

	hr = IWICBitmapFrameEncode_Initialize(piBitmapFrame, pPropertybag);
	if (FAILED(hr)) {
		IPropertyBag2_Release(pPropertybag);
		goto out;
	}
	IPropertyBag2_Release(pPropertybag);


	hr = IWICBitmapFrameEncode_SetSize(piBitmapFrame, dwWidth, dwHeight);
	if (FAILED(hr))
		goto out;

	hr = IWICImagingFactory_CreateFormatConverter(piFactory, &piFormatConverter);
	if (FAILED(hr))
		goto out;

        // Initialize the format converter.

	hr = IWICFormatConverter_QueryInterface(piFormatConverter, &IID_IWICBitmapSource, (void **)&piBitmapTmp);
        if (FAILED(hr))
		goto out;


	hr = IWICImagingFactory_CreateBitmapFromMemory(piFactory, dwWidth, dwHeight, &GUID_WICPixelFormat24bppBGR, dwWidth * 3, dwHeight * dwWidth * 3, pBuffer, &piBitmapSrc);
	if (FAILED(hr))
		goto out;

	hr = IWICFormatConverter_Initialize(piFormatConverter, (IWICBitmapSource *)piBitmapSrc, &GUID_WICPixelFormat24bppBGR,
			WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
		goto out;


	hr = IWICBitmapFrameEncode_WriteSource(piBitmapFrame, piBitmapTmp, NULL);
        if (FAILED(hr))
		goto out;

	hr = IWICBitmapFrameEncode_Commit(piBitmapFrame);
        if (FAILED(hr))
		goto out;

	hr = IWICBitmapEncoder_Commit(piEncoder);
        if (FAILED(hr))
		goto out;

	
#if 0
	printf("size = %lu\n", GlobalSize(hMem));
	FILE *fp = fopen("xxx.jpg", "wb");
	void *p = GlobalLock(hMem);
	fwrite(p, 1, GlobalSize(hMem), fp);
	fclose(fp);
	GlobalUnlock(hMem);
#endif

	iRetVal = 0;
out:

        if (piBitmapTmp)
		IWICBitmapSource_Release(piBitmapTmp);

        if (piFormatConverter)
		IWICFormatConverter_Release(piFormatConverter);

        if (piBitmapSrc)
		IWICBitmap_Release(piBitmapSrc);

        if (piEncoder)
		IWICBitmapEncoder_Release(piEncoder);

	if (piBitmapFrame)
		IWICBitmapFrameEncode_Release(piBitmapFrame);


        if (piFactory)
		IWICImagingFactory_Release(piFactory);

	if (piStream)
		IStream_Release(piStream);
	return iRetVal;
}

#if 0
int main(int argc, char **argv)
{

	HRESULT hr;
	WebcamInitialize();
	LINK list;
	WebcamListDevice(GetProcessHeap(), &list);
	LPLINK p;
	ListForEach(&list, p) {
		LPWEBCAM_NAME pName =ListGetEntry(p, WEBCAM_NAME, link);
		printf("%ls\n", pName->wzName);

	}
	LPWEBCAM pCam = WebcamOpen(GetProcessHeap(), 0);
	HGLOBAL gMem = GlobalAlloc(GMEM_MOVEABLE, 0);
	WebcamCaptureJpeg(pCam, gMem);
	Sleep(1000);
	WebcamCaptureJpeg(pCam, gMem);
	Sleep(1000);
	WebcamCaptureJpeg(pCam, gMem);
	Sleep(1000);
	// WebcamClose(pCam);
	WebcamUninitialize();
	return 0;
}
#endif
