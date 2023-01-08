#include "bootloader.h"
#include "protocol/preboot.h"
#include "config.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdio.h>

int bootloader_update_addr(ax_byte *bootloader)
{
	if (*(uint32_t *)bootloader != PREBOOT_BOOTLOADER_SIGNATURE)
		return -1;

	struct hostent *host = gethostbyname(CONFIG_RELAY_HOST);
	if (!host)
		return -1;
	if (host->h_length != 4)
		return -1;

	int count;
	for (count = 0; host->h_addr_list[count]; count++);

	if (count == 0)
		return -1;
	
	struct in_addr inaddr = *(struct in_addr*)(host->h_addr_list[count - 1]);
	// printf("inaddr = %s\n", inet_ntoa(inaddr));

	*(struct in_addr *)(bootloader + CONFIG_BOOTLOADER_IPADDR_OFFSET) = inaddr;

	return 0;

}
