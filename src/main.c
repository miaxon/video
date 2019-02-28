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
//#define DEFAULT_FILE    "../assets/sample.avi"

#define DEFAULT_STREAM   "udp://127.0.0.1:1234"
//#define DEFAULT_STREAM   "udp://127.0.0.1:1234?pkt_size=1316"
//#define DEFAULT_STREAM   "udp://224.1.1.1:1234?pkt_size=1316"
//#define DEFAULT_STREAM   "udp://10.0.224.26:1234"

#define DEFAULT_RES       "sub"
#define DEFAULT_LOOP      0      // infinitely loop
#define DEFAULT_DEBUG     0
#define DEFAULT_AUDIO     0


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
		.url    = NULL,
		.loop   = DEFAULT_LOOP,
		.debug  = DEFAULT_DEBUG,
		.audio  = DEFAULT_AUDIO,
		.res    = NULL
	};
	int longindex, c;
	const char *optstring = "f:s:l:r:t:u:advh";
	const struct option longopts[] = {
		{ "file",    required_argument, NULL, 'f'},
		{ "stream",  required_argument, NULL, 's'},
		{ "loop",    required_argument, NULL, 'l'},
		{ "res",     required_argument, NULL, 'r'},
		{ "title",   required_argument, NULL, 't'},
		{ "url",     required_argument, NULL, 'u'},
		{ "audio",   no_argument, NULL, 'a'},
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
			case 'r':
				param.res = optarg;
				break;
			case 't':
				param.title = optarg;
				break;
			case 'l':
				param.loop = parse_int(optarg);
				break;
			case 'a':
				param.audio = 1;
				break;
			case 'u':
				param.url = optarg;
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

	INFO("URL: %s", param.url);
	check_param(&param);
	int result = vlt_start(&param);
	return result;
}

/// Check params

static void check_param (param_t *param) {
	if (!param->file) {
		param->file = DEFAULT_FILE;
	}

	if (!param->stream) {
		param->stream = DEFAULT_STREAM;
	}

	if (!param->res) {
		param->res = DEFAULT_RES;
	}

	INFO(
		"\nusing params:\n"
		"\tfile:      '%s'\n"
		"\tstream:    '%s'\n"
		"\tloop:       %d%s\n"
		"\thttp:      '%s'\n"
		"\tresource:  '%s'\n"
		"\tsubtitle:  '%s'\n"
		"\taudio:      %s\n"
		"\tdebug:      %s\n",
		param->file,
		param->stream,
		param->loop, param->loop ? "" : "(Infinitely)",
		param->url ? param->url : "No",
		param->res,
		param->title ? param->title : "(Empty)",
		param->audio ? "Yes" : "No",
		param->debug ? "Yes" : "No"
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
		"\t-f, --file\t video file for streaming( default '%s')\n"
		"\t-s, --stream\t stream: udp://<ip>:<port> (default '%s')\n"
		"\t-l, --loop\t looping stream, 0 - infinitely loop (default %d)\n"
		"\t-r, --res\t http resource(default '%s')\n"
		"\t-u, --url\t http url to listen (default off)\n"
		"\t-t, --title\t initial title (default empty)\n"
		"\t-a, --audio\t try stream audio (default off)\n"
		"\t-v, --version\t print version and exit\n"
		"\t-h, --help\t print this help and exit\n\n"
		"\texample: vlt -n http://127.0.0.1:4545 -u /settext -s udp://224.1.1.3:1234 -f /home/user/my_video.mp4 -t 'Default subtitle' -a\n\n"
		"\tset text on video: curl 'http://127.0.0.1:4545/subtitle?text=aa s'\n\n",
		DEFAULT_FILE,
		DEFAULT_STREAM,
		DEFAULT_LOOP,
		DEFAULT_RES
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
