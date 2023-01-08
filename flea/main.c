#include "tunnel.h"
#include "modset.h"
#include "config.h"

#include <ax/option.h>
#include <ax/mem.h>

#include <unistd.h>
#include <sys/wait.h>
#include <err.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define MODULE_DIR_PATH "../modules"

void usage()
{
	fputs("Usage: flea <COMMAND> ...\n\n"
	      "Commands:\n"
	      "  lshost    print all active hosts\n"
	      "  rmhost    remove specified hosts\n"
	      "  lsmod     print all active modules\n"
	      "  insmod    install module to specified host\n"
	      "  rmmod     remove module from specified host\n"
	      "  sysinfo   print system information for remote host\n"
	      "  run       run local module\n"
	      , stderr);
}

extern int cmd_lshost(int argc, char *argv[]);
extern int cmd_rmhost(int argc, char *argv[]);
extern int cmd_lsmod(int argc, char *argv[]);
extern int cmd_insmod(int argc, char *argv[]);
extern int cmd_rmmod(int argc, char *argv[]);
extern int cmd_sysinfo(int argc, char *argv[]);
extern int cmd_run(int argc, char *argv[]);

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		exit(EXIT_FAILURE);
	}
	if (modset_init(MODULE_DIR_PATH)) {
		fprintf(stderr, "Failed to load modules: %s", strerror(errno));
		exit(EXIT_FAILURE);
		
	}

	if (strcmp(argv[1], "lshost") == 0)
		return cmd_lshost(argc - 1, argv + 1);
	if (strcmp(argv[1], "rmhost") == 0)
		return cmd_rmhost(argc - 1, argv + 1);
	if (strcmp(argv[1], "lsmod") == 0)
		return cmd_lsmod(argc - 1, argv + 1);
	if (strcmp(argv[1], "insmod") == 0)
		return cmd_insmod(argc - 1, argv + 1);
	if (strcmp(argv[1], "rmmod") == 0)
		return cmd_rmmod(argc - 1, argv + 1);
	if (strcmp(argv[1], "sysinfo") == 0)
		return cmd_sysinfo(argc - 1, argv + 1);
	if (strcmp(argv[1], "run") == 0)
		return cmd_run(argc - 1, argv + 1);
	if (strcmp(argv[1], "-h") == 0) {
		usage();
		return 0;
	}
	fprintf(stderr, "Unsupported command '%s'\n", argv[1]);
	usage();
	exit(EXIT_FAILURE);
}

