#include "mux_inp.h"
#include "err.h"
#include "log.h"

muxer_t*
mux_inp_new (const char* name) {
	muxer_t *mux = NULL;
	int	ret = 0;

	if ((mux = (muxer_t*)av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ((ret = avformat_open_input(&mux->ctx_format, name, NULL, NULL)) != 0) {
		ERR_EXIT("'%s' failed", "avformat_open_input");
	}

	if (mux->ctx_format == NULL) {
		ERR_EXIT("%s", "Could not create input context");
	}

	if ((ret = avformat_find_stream_info(mux->ctx_format, 0)) < 0) {
		ERR_EXIT("'%s' failed", "avformat_find_stream_info");
	}

	if ((mux->index_video = av_find_best_stream(mux->ctx_format, AVMEDIA_TYPE_VIDEO, -1, -1, &mux->codec_video, 0)) == -1) {
		ERR_EXIT("%s", "Didn't find a video input stream");
	}

	if ((mux->index_audio = av_find_best_stream(mux->ctx_format, AVMEDIA_TYPE_AUDIO, -1, -1, &mux->codec_audio, 0)) == -1) {
		ERR_EXIT("%s", "Didn't find a video input stream");
	}

	mux->stream_audio = mux->ctx_format->streams[mux->index_audio];
	mux->param_audio = mux->stream_audio->codecpar;
	mux->stream_video = mux->ctx_format->streams[mux->index_video];
	mux->param_video = mux->stream_video->codecpar;

	// Allocate decoder
	if ((mux->codec_video = avcodec_find_decoder(mux->param_video->codec_id)) == NULL) {
		ERR_EXIT("'%s' failed", "Didn't find a video input stream");
	}

	if ((mux->ctx_codec_video = avcodec_alloc_context3(mux->codec_video)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_alloc_context3");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_codec_video, mux->stream_video->codecpar)) < 0) {
		ERR_EXIT("'%s' failed", "avcodec_parameters_to_context");
	}

	if ((ret = avcodec_open2(mux->ctx_codec_video, mux->codec_video, NULL)) < 0) {
		ERR_EXIT("'%s' failed", "avcodec_open2");
	}


	INFO("%s" , "dump input format");
	av_dump_format(mux->ctx_format, 0, name, 0);

	return mux;

}

void
mux_inp_free (muxer_t* mux) {

}