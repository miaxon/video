#include "mux_out.h"
#include "err.h"
#include "log.h"

muxer_t*
mux_out_new (const char* name, const char *format, int video_codec_id, int audio_codec_id) {
	muxer_t *mux = NULL;
	int ret = 0;

	if ((mux = (muxer_t*)av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ((ret = avformat_alloc_output_context2(&mux->ctx_format, NULL, format, name)) < 0) {
		ERR_EXIT("'%s' failed", "avformat_alloc_output_context2");
	}

	if (mux->ctx_format == NULL) {
		ERR_EXIT("%s", "Could not create output context");
	}

	/// Add video stream
	if ((mux->codec_video = avcodec_find_encoder(video_codec_id)) == NULL) {
		ERR_EXIT("%s", "'avcodec_find_encoder' failed");
	}

	if ((mux->ctx_codec_video = avcodec_alloc_context3(mux->codec_video)) == NULL) {
		ERR_EXIT("%s", "'avcodec_alloc_context3' failed");
	}

	if ((mux->stream_video = avformat_new_stream(mux->ctx_format, mux->codec_video)) == NULL) {
		ERR_EXIT("%s", "'avformat_new_stream' failed");
	}

	/// Add audio stream
	if ((mux->codec_audio = avcodec_find_encoder(audio_codec_id)) == NULL) {
		ERR_EXIT("%s", "'avcodec_find_encoder' failed");
	}

	if ((mux->ctx_codec_audio = avcodec_alloc_context3(mux->codec_audio)) == NULL) {
		ERR_EXIT("%s", "'avcodec_alloc_context3' failed");
	}

	if ((mux->stream_audio = avformat_new_stream(mux->ctx_format, mux->codec_audio)) == NULL) {
		ERR_EXIT("%s", "'avformat_new_stream' failed");
	}

	INFO("%s" , "dump output format");
	av_dump_format(mux->ctx_format, 0, name, 1);

	return mux;
}

void
mux_out_free (muxer_t* mux) {

}