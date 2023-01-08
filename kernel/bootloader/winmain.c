#include <stdio.h>
#include <windows.h>
extern unsigned char bootloader_bin[];
extern unsigned int bootloader_bin_len;
int main(){
	char *p = malloc(bootloader_bin_len);
	memcpy(p, bootloader_bin, bootloader_bin_len);
	DWORD dwOldProt = 0;
	int ret = VirtualProtect(p, bootloader_bin_len, PAGE_EXECUTE_READWRITE, &dwOldProt);
	return ((int(*)())p)();
}
