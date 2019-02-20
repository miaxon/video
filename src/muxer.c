#include <libavutil/imgutils.h>

#include "muxer.h"
#include "err.h"
#include "log.h"
#include "utils.h"

#define OUTPUT_FORMAT     "mpegts"
#define OUTPUT_PIXFMT    AV_PIX_FMT_YUV420P
#define OUTPUT_ENCODER    AV_CODEC_ID_H264
#define OUTPUT_BITRATE    150 * 1000
#define OUTPUT_FRAMERATE  30
#define OUTPUT_GOPSIZE    12

muxer_t *
muxer_new (const char* name, int width, int height) {
	muxer_t *mux = NULL;
	int ret = 0;

	if ((mux = (muxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ((ret = avformat_alloc_output_context2(&mux->ctx_f, NULL, OUTPUT_FORMAT, name)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_alloc_output_context2", av_err2str(ret));
	}

	if (mux->ctx_f == NULL) {
		ERR_EXIT("%s", "Could not create output context");
	}

	if ((mux->cv = avcodec_find_encoder(OUTPUT_ENCODER)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_find_encoder");
	}

	if ((mux->sv = avformat_new_stream(mux->ctx_f, mux->cv)) == NULL) {
		ERR_EXIT("'%s' failed", "avformat_new_stream");
	}

	if ((mux->ctx_cv = avcodec_alloc_context3(mux->cv)) == NULL) {
		ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_cv, mux->sv->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	mux->width   = width;
	mux->height  = height;
	mux->pix_fmt = OUTPUT_PIXFMT;

	mux->ctx_cv->bit_rate   = OUTPUT_BITRATE;
	mux->ctx_cv->width      = width;
	mux->ctx_cv->height     = height;
	mux->ctx_cv->time_base  = (AVRational){1, OUTPUT_FRAMERATE};
	mux->ctx_cv->gop_size   = OUTPUT_GOPSIZE;
	mux->ctx_cv->pix_fmt    = OUTPUT_PIXFMT;

	if ((ret = avcodec_open2(mux->ctx_cv, mux->cv, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}

	if (!(mux->ctx_f->oformat->flags & AVFMT_NOFILE)) {
		if ( (ret = avio_open(&mux->ctx_f->pb, name, AVIO_FLAG_WRITE)) < 0) {
			ERR_EXIT("'%s' failed: %s", "avio_open", av_err2str(ret));
		}
	}

	if ( (ret = avformat_write_header(mux->ctx_f, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_write_header", av_err2str(ret));
	}

	av_dump_format(mux->ctx_f, 0, name, 1);

	return mux;
}

void
muxer_free (muxer_t* mux) {
	avcodec_close(mux->ctx_cv);
	avio_close(mux->ctx_f->pb);
	av_free(mux->ctx_f);
}