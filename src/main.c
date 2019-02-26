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

//#define DEFAULT_FILE    "/mnt/WDC/CINEMA/er/ER_s01/ER.01x01.-.24.hours.rus.avi"
#define DEFAULT_FILE    "../assets/sample.mp4"
//#define DEFAULT_STREAM   "udp://127.0.0.1:1234"
//#define DEFAULT_STREAM   "udp://127.0.0.1:1234?pkt_size=1316"
//#define DEFAULT_STREAM   "udp://224.1.1.1:1234?pkt_size=1316"
#define DEFAULT_STREAM   "udp://10.0.224.26:1234"

#define DEFAULT_PORT     4535
#define DEFAULT_URL      "/subtitle"
#define DEFAULT_LOOP     0

_Noreturn static void print_usage (void);
_Noreturn static void print_version (void);

static void check_param (param_t *param);
/// Application entry point
static int parse_int (const char *str);

/**
 * Main function that parse command line params and then start
 */
int
main (int argc, char **argv) {

	param_t param = {
		.file   = NULL,
		.stream = NULL,
		.title  = NULL,
		.url    = DEFAULT_URL,
		.loop   = DEFAULT_LOOP,
		.port   = DEFAULT_PORT,
		.debug  = 0
	};

	int longindex, c;
	const char *optstring = "f:s:l:p:u:t:dvh";
	const struct option longopts[] = {
		{ "file",    required_argument, NULL, 'f'},
		{ "stream",  required_argument, NULL, 's'},
		{ "loop",    required_argument, NULL, 'l'},
		{ "port",    required_argument, NULL, 'p'},
		{ "url",     required_argument, NULL, 'u'},
		{ "title",   required_argument, NULL, 't'},
		{ "debug",   no_argument, NULL, 'd'},
		{ "version", no_argument, NULL, 'v'},
		{ "help",    no_argument, NULL, 'h'},
		{ 0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, optstring, longopts, &longindex)) != -1) {
		switch (c) {
			case 's':
				param.stream = optarg;
				break;
			case 'f':
				param.file = optarg;
				break;
			case 'u':
				param.url = optarg;
				break;
			case 't':
				param.title = optarg;
				break;
			case 'p':
				param.port = parse_int(optarg);
				break;
			case 'l':
				param.loop = parse_int(optarg);
				break;
			case 'd':
				param.debug = 1;
				break;
			case 'v':
				print_version();
				break;
			case 'h':
				print_usage();
				break;
			default:
				print_usage();
		};
	};


	check_param(&param);

	int result = vlt_start(&param);
	result = vlt_start(&param);
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

	if (param->port < 1024 || param->port > 65535)
		ERR_EXIT("%s", "port value must be from 1024 to 65535");

	if (param->loop < 0)
		ERR_EXIT("%s", "loop value must be positive");

	INFO(
			"\nusing params:\n"
			"\tfile   \t'%s'\n"
			"\tstream \t'%s'\n"
			"\tport   \t'%d'\n"
			"\tloop   \t'%d'\n"
			"\turl    \t'%s'\n"
			"\ttitle  \t'%s'\n",
			"\tdebug  \t'%s'\n",
			param->file,
			param->stream,
			param->port,
			param->loop,
			param->url,
			param->title,
			param->debug ? "Yes" : "Mo"
			);
}

static int
parse_int (const char* src) {
	char *end;
	const long val = strtol(src, &end, 0);

	if (end == src || *end != '\0')
		err_exit("Invalid argument");

	return val;
}

/// Print usage info and exit

_Noreturn static void
print_usage (void) {
	printf(
			"Usage: vlt [slputfvh] [OPTION]\n"
			"\t-s, --stream\t stream on udp://<ip>:<port> (default '"DEFAULT_STREAM"')\n"
			"\t-l, --loop\t looping stream, 0 - infinitely looping (default 0)\n"
			"\t-p, --port\t http port to listen (default '4535')\n"
			"\t-u, --url\t url to listen(default '"DEFAULT_URL"'\n"
			"\t-t, --title\t initial title (default empty)\n"
			"\t-f, --file\t video file for streaming( default '"DEFAULT_FILE"')\n"
			"\t-v, --version\t print version and exit\n"
			"\t-h, --help\t print this help and exit\n\n"
			"\texample: vlt -p 1333 -u /settext -s udp://224.1.1.3:1234 -f /home/user/my_video.mp4\n\n"
			);
	exit(1);
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
