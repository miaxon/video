
#ifndef MUXER_H
#define MUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>	

	typedef struct {
		AVFormatContext   *ctx_f;
		AVCodecContext    *ctx_cv;
		AVCodecContext    *ctx_ca;
		AVCodec           *cv;
		AVCodec           *ca;
		AVStream          *sv;
		AVStream          *sa;
		AVCodecParameters *pv;
		AVCodecParameters *pa;
		int width;
		int height;
		int pix_fmt;
	} muxer_t;

	muxer_t *
	muxer_new(const char* name, int width, int height);

	void
	muxer_free(muxer_t* mux);
	
	


#ifdef __cplusplus
}
#endif

#endif /* MUXER_H */

