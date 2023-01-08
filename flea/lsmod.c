#include "tunnel.h"
#include "modset.h"

#include <ax/option.h>
#include <ax/mem.h>

#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


void usage_lsmod()
{
	fputs("Usage: flea lsmod OPTIONS...\n\n"
	      "Options:\n"
	      "    Print all modules that zombie host have installed.\n\n"
	      "    -r <address>    specifies relay server address\n"
	      "    -p <keyword>    specifies relay keyword\n"
	      "    -h              display this help\n"
	, stderr);
}

int cmd_lsmod(int argc, char **argv)
{
	return 0;
}
