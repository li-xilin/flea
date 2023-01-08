#include "library.h"
#include <ax/detect.h>
#include <dlfcn.h>

#if defined(AX_OS_WIN32)
static char last_error_message_win32[256];
static void library_win32_seterror(void)
{
	DWORD errcode = GetLastError();
	if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, errcode, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
			last_error_message_win32, sizeof(last_error_message_win32)-1, NULL)) {
		sprintf(last_error_message_win32, "unknown error %lu", errcode);
	}
}

#endif

library library_open(const char* fname)
{
#if defined(AX_OS_WIN32)
	HMODULE h;
	int emd;
	emd = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	h = LoadLibrary(fname);
	SetErrorMode(emd);
	if(!h) {
		library_win32_seterror();
		return NULL;
	}
	last_error_message_win32[0] = 0;
	return (dlib)h;
#else
	return (library )dlopen(fname, RTLD_NOW | RTLD_GLOBAL);
#endif
}

void *library_symbol(library l, const char *func)
{
#if defined(AX_OS_WIN32)
	void *ptr;
	*(FARPROC*)(&ptr) = GetProcAddress((HMODULE)l, func);
	if(!ptr)
	{
		library_win32_seterror();
		return NULL;
	}
	last_error_message_win32[0] = 0;
	return ptr;
#else
	return dlsym((void *)l, func);
#endif
}

int library_close(library l)
{
#if defined(AX_OS_WIN32)
	if(!FreeLibrary((HMODULE) l)) {
		library_win32_seterror();
		return -1;
	}
	last_error_message_win32[0] = 0;
	return 0;
#else
	return - !!dlclose((void *)l);
#endif
}

char *library_error(void)
{
#if defined(AX_OS_WIN32)
	if (last_error_message_win32[0])
		return last_error_message_win32;
	else
		return NULL;
#else
	return dlerror();
#endif
}
