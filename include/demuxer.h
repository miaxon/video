

#ifndef DEMUXER_H
#define DEMUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

	typedef struct {
		AVFormatContext *ctx_f;
		AVCodecContext *ctx_cv;
		AVCodecContext *ctx_ca;
		AVCodec *cv;
		AVCodec *ca;
		AVStream *sv;
		AVStream *sa;
		AVCodecParameters *pv;
		AVCodecParameters *pa;
		int siv;
		int sia;
		int width;
		int height;
	} demuxer_t;

	demuxer_t *
	demuxer_new(const char* name);

	void
	demuxer_free(demuxer_t* mux);

#ifdef __cplusplus
}
#endif

#endif /* DEMUXER_H */

