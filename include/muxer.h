
#ifndef MUXER_H
#define MUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h> 

	typedef struct {
		AVFormatContext   *ctx_f;  // format    format context
		AVCodecContext    *ctx_cv; // video     codec  context
		AVCodecContext    *ctx_ca; // audio
                AVCodecContext    *ctx_cb; // subdvb
		AVCodec           *cv;     // video     codec
		AVCodec           *ca;     // audio
                AVCodec           *cb;     // subdvb
		AVStream          *sv;     // video     stream
		AVStream          *sa;     // audio
                AVStream          *sb;     // subdvb
		AVCodecParameters *pv;	   // video     codec params
		AVCodecParameters *pa;	   // audio
                AVCodecParameters *pb;	   // subdvb
		int                fc;     // frame count

		int width;
		int height;
		int pix_fmt;
	} muxer_t;

	muxer_t *
	muxer_new(const char* name, int width, int height);

	void
	muxer_free(muxer_t* mux);

	int
	muxer_encode_frame(muxer_t *mux, AVFrame *src);
	
	int
	muxer_encode_subtitle(muxer_t *mux, char *text);
	


#ifdef __cplusplus
}
#endif

#endif /* MUXER_H */

