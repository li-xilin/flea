#include "config.h"
#include "tunnel.h"
#include "modset.h"
#include "library.h"

#include <ax/option.h>
#include <ax/mem.h>
#include <ax/option.h>

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <errno.h>

typedef int flea_start_fn(const char *modname, const char *keyword, const char *relay_host, uint16_t port,char *arg_line);
typedef int flea_stop_fn();

static flea_stop_fn *g_stop_fn;

static void signal_handler (int signal);
static int register_signal();

static void signal_handler (int sig)
{
	if (sig == SIGINT) {
		g_stop_fn();
	}
}

static int register_signal()
{
        struct sigaction sa = {
                .sa_handler = signal_handler,
                .sa_flags = SA_NOMASK,
        };
        if (sigaction(SIGINT, &sa, NULL))
                return -1;

        // if (sigaction(SIGTERM, &sa, NULL))
        //         return -1;
        return 0;
}

static void usage()
{
        fputs("Usage: flea run OPTIONS... MODULE...\n\n"
              "Options:\n"
              "    Run local MODULE.\n\n"
              "    -r <address>    specifies relay server address\n"
              "    -p <keyword>    specifies relay keyword\n"
              "    -h <host>       Remote zombie host id\n"
              "    -a <arguments>  specifies argument pass to module\n"
              "    -h              Display this help\n"
        , stderr);
}

int cmd_run(int argc, char **argv)
{
	struct ax_option_st options;
	ax_option_init(&options, argv);
	const char *relay_addr = NULL;
	const char *keyword = NULL;
	char *sub_arg = NULL;
	for (int op; (op = ax_option_parse(&options, "r:p:a:h")) != -1;) {
		switch (op) {
			case 'r':
				relay_addr = options.optarg;
				break;
			case 'p':
				keyword = options.optarg;
				break;
			case 'a':
				sub_arg = options.optarg;
				break;
			case 'h':
				return 0;
			default:
				fprintf(stderr, "Unsupported option -%c\n", options.optopt);
				usage();
				return 1;
		}
	}

	if (!relay_addr) {
		ax_perror("Option -r expected");
		return 1;
	}

	if (!keyword) {
		fprintf(stderr, "Option -p expected");
		return 1;
	}

	if (argc - options.optind == 0) {
		fprintf(stderr, "Module name expected");
		usage();
		return 1;
	}

	struct mod *mod = modset_find(argv[options.optind]);
	if (!mod)
		return 1;

	if (register_signal()) {
		fprintf(stderr, "%s: failed to register SIGINT handler, %s\n", argv[0], strerror(errno));
		return 1;
	}

	char client_path[PATH_MAX];
	strcpy(client_path, modset_dir());
	strcat(client_path, "/");
	strcat(client_path, mod->directory);
	strcat(client_path, "/");
	strcat(client_path, mod->client_mod);

	library lib = library_open(client_path);
	if (lib == 0)
		return 1;

	flea_start_fn *start_fn = (flea_start_fn *)(uintptr_t)library_symbol(lib, "flea_start");
	if (!start_fn) {
		fprintf(stderr, "%s, symbol flea_start is not found\n", mod->name);
		library_close(lib);
		return 1;
	}

	flea_stop_fn *stop_fn = (flea_stop_fn *)(uintptr_t)library_symbol(lib, "flea_stop");
	if (!stop_fn) {
		fprintf(stderr, "%s, symbol flea_stop is not found\n", mod->name);
		library_close(lib);
		return 1;
	}
	g_stop_fn = stop_fn;

	int ret = !!start_fn(mod->name, keyword, relay_addr, 6666, sub_arg);
	library_close(lib);

	return ret;
}
