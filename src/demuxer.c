#include "demuxer.h"
#include "err.h"
#include "log.h"
#include "utils.h"

demuxer_t *
demuxer_new(const char* name) {
	demuxer_t * mux;
	int	ret = 0;

	if ((mux = (demuxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ((ret = avformat_open_input(&mux->ctx_f, name, NULL, NULL)) != 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_open_input", av_err2str(ret));
	}

	if (mux->ctx_f == NULL) {
		ERR_EXIT("%s", "Could not create input context");
	}

	if ((ret = avformat_find_stream_info(mux->ctx_f, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_find_stream_info", av_err2str(ret));
	}

	if ((mux->siv = av_find_best_stream(mux->ctx_f, AVMEDIA_TYPE_VIDEO, -1, -1, &mux->cv, 0)) == -1) {
		ERR_EXIT("%s", "Didn't find a video input stream");
	}

	if (mux->cv == NULL) {
		ERR_EXIT("'%s' failed", "Didn't find a video input decoder");
	}

	mux->sv = mux->ctx_f->streams[mux->siv];
	mux->pv = mux->sv->codecpar;

	if ((mux->ctx_cv = avcodec_alloc_context3(mux->cv)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_alloc_context3");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_cv, mux->pv)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	if (mux->ctx_cv == NULL) {
		ERR_EXIT("'%s' failed", "Didn't have video code context");
	}

	if ((ret = avcodec_open2(mux->ctx_cv, mux->cv, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}
	
	mux->width = mux->ctx_cv->width;
	mux->height = mux->ctx_cv->height;
	
	INFO("%s" , "dump input format");
	av_dump_format(mux->ctx_f, 0, name, 0);
	return mux;
}

void
demuxer_free(demuxer_t* mux) {
}