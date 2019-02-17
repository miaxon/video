/**
 * \file main
 */
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "err.h"
#include "log.h"
#include "vlt.h"

#define DEFAULT_FILE    "../assets/sample.mp4"
//#define DEFAULT_STREAM   "udp://127.0.0.1:1234"
#define DEFAULT_STREAM   "udp://10.0.224.26:1234"

_Noreturn static void err_usage (void);
_Noreturn static void print_version (void);

static void check_param (param_t *param);
/// Application entry point

/**
 * Main function that parse command line params and then start
 */
int
main (int argc, char **argv) {

	param_t param = {
		.file   = NULL,
		.stream = NULL
	};

	int longindex, c;
	const char *optstring = "f:l:s:vh";
	const struct option longopts[] = {
		{ "file",    required_argument, NULL, 'f'},
		{ "stream",  required_argument, NULL, 's'},
		{ "listen",  required_argument, NULL, 'l'},
		{ "version", no_argument, NULL, 'v'},
		{ "help",    no_argument, NULL, 'h'},
		{ 0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, optstring, longopts, &longindex)) != -1) {
		switch (c) {
			case 's':

				break;
			case 'f':
				param.file = optarg;
				break;
			case 'v':
				print_version();
				break;
			default:
				err_usage();
		};
	};

	log_start(LOG_OPTS, LOG_FACILITY);

	check_param(&param);
	int result = vlt_start(&param);

	log_stop();
	return !result;
}

/// Check params

static void check_param (param_t *param) {
	if (!param->file) {
		param->file = DEFAULT_FILE;
	}
	if (!param->stream) {
		param->stream = DEFAULT_STREAM;
	}
	INFO(
			"\nusing params:\n"
			"\tfile   '%s'\n"
			"\tstream '%s'\n",
			param->file,
			param->stream
			);
}

/// Print usage info and exit

_Noreturn static void
err_usage (void) {
	printf(
			"Usage: vlt [slf] [OPTION]\n"
			"\t-s, --stream\t stream on <ip>:<port>\n"
			"\t-l, --listen\t listen port\n"
			"\t-f, --file\t video file for streaming\n"
			"\t-v, --version\t print version and exit\n"
			"\t-h, --help\t print this help and exit\n"
			);
	err_exit("exit");
}

/// Debug info

/**
 * COMMIT and DATE are compile time constants (see Makefile)
 */
_Noreturn static void
print_version (void) {
	printf("build %s %s\n", COMMIT, DATE);
	err_exit("exit");
}
