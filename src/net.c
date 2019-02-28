#include <pthread.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>


#include "net.h"
#include "log.h"
#include "err.h"
#include "urldec.h"

#define SUB_SIZE 256

static char sub[SUB_SIZE];
static char txt[SUB_SIZE]; // static pointer to currenet subtitle text;
static char *url;
static char *res;
static pthread_mutex_t lock;

static unsigned const char *html = (unsigned char*) \
"<!DOCTYPE html>"\
"<html>"\
"<body>"\
"<h4>Set subtitle on video:</h4>"\
"<form accept-charset='utf-8' action='/subtitle'>"\
"<p>Note 1: input text is 256 characher's line.</p>"\
"<p>Note 2: use '\\N' as newline delimiter.</p>"\
"Text:<br>"\
"<input type='text' name='text'>"\
"<br><br>"\
"<input type='submit' value='Submit'>"\
"</form>"\
"</body>"\
"</html>"\
;

typedef void * (* entry_t)(void *) ;
static void  net_thread(void *param);
static int   net_client(AVIOContext *client);
static void  net_set_subtitle(const char* text);

int
net_init(char *t, char *u, char *r) {
	int ret = 0;
	pthread_t th;
	pthread_attr_t attr;
	memset(sub, 0, SUB_SIZE);
	strncpy(sub, t, SUB_SIZE - 1);
	url  = u;
	res  = r;
	entry_t entry = (entry_t) net_thread;

	if ((ret = pthread_attr_init(&attr)) != 0) {
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_attr_init");
	}

	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_attr_setdetachstate");
	}

	if ((ret = pthread_mutex_init(&lock, NULL)) != 0) {
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_init");
	}

	if ((ret = pthread_create(&th, NULL, entry, NULL)) != 0)
		err_sys("Could note create web thread");

	if ((ret = pthread_attr_destroy(&attr)) != 0) {
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_attr_destroy");
	}

	//pthread_join(th, NULL);
	return 0;
}

static void
net_set_subtitle(const char* text) {
	if (pthread_mutex_lock(&lock) != 0)
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_lock");

	memset(sub, 0, SUB_SIZE);
	strncpy(sub, text, SUB_SIZE - 1);
	INFO("HTTP:set new subtitle text '%s'", sub);

	if (pthread_mutex_unlock(&lock) != 0)
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_unlock");
}

char*
net_subtitle(void) {
	if (pthread_mutex_lock(&lock) != 0)
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_lock");

	memset(txt, 0, SUB_SIZE);
	strncpy(txt, sub, SUB_SIZE - 1);

	if (pthread_mutex_unlock(&lock) != 0)
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_unlock");

	return txt;
}

static void
net_thread(void *param) {
	//av_log_set_level(AV_LOG_TRACE);
	int ret = 0;
	AVDictionary *opts = NULL;
	AVIOContext *client = NULL, *server = NULL;

	if ((ret = av_dict_set(&opts, "listen", "2", 0)) < 0) {
		ERR_EXIT("HTTP:'%s' failed: %s", "av_dict_set", av_err2str(ret));
	}

	if ((ret = avio_open2(&server, url, AVIO_FLAG_WRITE, NULL, &opts)) < 0) {
		ERR_EXIT("HTTP:'%s' failed: %s", "avio_open2", av_err2str(ret));
	}
	INFO("HTTP: start http listen: %s", url);
	for (; ; ) {
		if ((ret = avio_accept(server, &client)) < 0) {
			ERROR("HTTP:'%s' failed: %s", "avio_accept", av_err2str(ret));
			continue;
		}
		ret = net_client(client);
		if (ret < 0)
			break;
	}
	INFO("HTTP: stop http listen: %s", url);
	pthread_mutex_destroy(&lock);
	avio_close(server);
}

static int
net_client(AVIOContext *client) {
	int ret, reply_code;
	char *resource = NULL;

	while ((ret = avio_handshake(client)) > 0) {
		av_opt_get(client, "resource", AV_OPT_SEARCH_CHILDREN, (uint8_t**) & resource);
		if (resource && strlen((const char*) resource))
			break;
		av_freep(&resource);
	}
	if (ret < 0) {
		ERROR("HTTP:'%s' failed: %s", "avio_handshake", av_err2str(ret));
		return 0;
	}

	if (resource && resource[0] == '/' && !strncmp((resource + 1), res, strlen(res))) {

		reply_code = 200;

		if (url_decode(resource) == 0) {
			char *txt = strstr(resource, "?text=");
			if (txt) {
				net_set_subtitle(strstr(resource, "=") + 1);
			} else {
				ERROR("Invalid parameters string '%s'", resource);
				reply_code = AVERROR_HTTP_BAD_REQUEST;
			}
		} else {
			ERROR("Failed to decode '%s'", resource);
			reply_code = AVERROR_HTTP_BAD_REQUEST;
		}
	} else {
		reply_code = AVERROR_HTTP_NOT_FOUND;
	}

	if (reply_code == 200) {
		if ((ret = av_opt_set(client, "location", "/subtitle", AV_OPT_SEARCH_CHILDREN)) < 0) {
			ERR_EXIT("HTTP:'%s' failed: %s", "av_opt_set_int", av_err2str(ret));
		}
	}
	if ((ret = av_opt_set(client, "content_type", "text/html", AV_OPT_SEARCH_CHILDREN)) < 0) {
		ERR_EXIT("HTTP:'%s' failed: %s", "av_opt_set_int", av_err2str(ret));
	}

	if ((ret = av_opt_set_int(client, "reply_code", reply_code, AV_OPT_SEARCH_CHILDREN)) < 0) {
		ERR_EXIT("HTTP:'%s' failed: %s", "av_opt_set_int", av_err2str(ret));
	}

	while ((ret = avio_handshake(client)) > 0);

	avio_write(client, html, strlen((const char*) html));
	av_freep(&resource);
	avio_flush(client);
	avio_close(client);
	return 0;
}