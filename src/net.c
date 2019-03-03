#include <pthread.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/avstring.h>


#include "net.h"
#include "log.h"
#include "err.h"
#include "urldec.h"

#define SUB_SIZE 256

#define LOCK(x) if (pthread_mutex_lock(&x) != 0)\
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_lock");
#define UNLOCK(x) if (pthread_mutex_unlock(&x) != 0)\
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_mutex_unlock");

static char sub[SUB_SIZE]; // provided for internal use
static char txt[SUB_SIZE]; // provided for external use
static char *url;
static char *res;
static unsigned const char *html;
static pthread_mutex_t mutex;

#define HTTP_FORM \
"<!DOCTYPE html>"\
"<html>"\
"<body>"\
"<h4>Set subtitle on video.</h4>"\
"<form id='form' accept-charset='utf-8' action='/%s'>"\
"<p>Note 1: input text must be not over 256 characher's length in 3 lines.</p>"\
"<p>Note 2: max lines number is 3.</p>"\
"<p>Note 2: use '\\N' as newline delimiter.</p>"\
"Text:<br>"\
"<textarea rows='4' cols='50' name='text' form='form'></textarea><br>"\
"<input type='submit' value='Submit'>"\
"</form>"\
"</body>"\
"</html>"\

typedef void * (* entry_t)(void *) ;
static void  net_thread(void *param);
static int   net_client(AVIOContext *client);
static void  net_set_subtitle(const char* text);
static int   net_parse_request(char *str);

void
net_clear(void) {
	LOCK(mutex)
	memset(sub, 0, SUB_SIZE);
	memset(txt, 0, SUB_SIZE);
	UNLOCK(mutex)
}

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

	if ((html = (unsigned char*)av_asprintf(HTTP_FORM, res)) == NULL) {
		ERR_EXIT("HTTP:'%s' failed: %s", "av_asprintf");
	}
	
	if ((ret = pthread_attr_init(&attr)) != 0) {
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_attr_init");
	}

	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		ERR_SYS("HTTP:'%s' failed: %s", "pthread_attr_setdetachstate");
	}

	if ((ret = pthread_mutex_init(&mutex, NULL)) != 0) {
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

static int
net_parse_request(char *str) {
	int len = strlen(res);
	int rlen = strlen(str + 1);
	int cmp = strncmp((str + 1), res, strlen(res));
	char *req = strstr(str, "?");

	if (!req && len == rlen && !strcmp(res, str + 1))
		return 200;
	
	if (req && (req  - (str + 1)) != len)
		return AVERROR_HTTP_NOT_FOUND;

	if (str && str[0] == '/' && !cmp) {
		if (len < rlen && url_decode(str) == 0) {
			char *txt = strstr(str, "?text=");
			if (txt) {
				net_set_subtitle(strstr(str, "=") + 1);
			}
		}
	}
	return 200;
}

static void
net_set_subtitle(const char* str) {
	LOCK(mutex)

	memset(sub, 0, SUB_SIZE);
	strncpy(sub, str, SUB_SIZE - 1);
	INFO("HTTP:set new subtitle text '%s'", sub);

	UNLOCK(mutex)
}

char*
net_subtitle(void) {
	LOCK(mutex)

	memset(txt, 0, SUB_SIZE);
	strncpy(txt, sub, SUB_SIZE - 1);

	UNLOCK(mutex)
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
	av_dict_free(&opts);
	INFO("HTTP: start http listen: %s", url);
	for (; ; ) {
		if ((ret = avio_accept(server, &client)) < 0) {
			ERROR("HTTP:'%s' failed: %s", "avio_accept", av_err2str(ret));
			continue;
		}
		ERROR("HTTP:'%s' failed: %s", "avio_accept", av_err2str(ret));
		ret = net_client(client);
		if (ret < 0)
			break;
	}
	INFO("HTTP: stop http listen: %s", url);
	
	pthread_mutex_destroy(&mutex);
	avio_close(server);
}

void
net_close(void){
	
}

static int
net_client(AVIOContext * client) {
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

	reply_code = net_parse_request(resource);
	av_freep(&resource);

	if (reply_code == 200) {
		if ((ret = av_opt_set(client, "location", "/sub", AV_OPT_SEARCH_CHILDREN)) < 0) {
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