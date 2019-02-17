#include <libavutil/opt.h>

#include "mux_out.h"
#include "err.h"
#include "log.h"

muxer_t*
mux_out_new (const char* name, const char *format, int video_codec_id, muxer_t *mux_inp)
{
	muxer_t *mux = NULL;
	int ret = 0;

	if ((mux = (muxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ((ret = avformat_alloc_output_context2(&mux->ctx_format, NULL, format, name)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_alloc_output_context2", av_err2str(ret));
	}

	if (mux->ctx_format == NULL) {
		ERR_EXIT("%s", "Could not create output context");
	}

	for (int i = 0; i < mux_inp->ctx_format->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream
			*in_stream  = NULL,
			*out_stream = NULL;

		in_stream  = mux_inp->ctx_format->streams[i];
		if(in_stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
			if ( (out_stream = avformat_new_stream(mux->ctx_format, NULL)) == NULL) {
				ERR_EXIT("%s", "Could not create output stream");
			}
			//Copy the settings of AVCodecContext
			if ( (ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar)) < 0) {
				ERR_EXIT("'%s' failed: %s", "avcodec_parameters_copy", av_err2str(ret));
			}

		} else {
			if ((mux->codec_video = avcodec_find_encoder(video_codec_id)) == NULL) {
				ERR_EXIT("%s", "'avcodec_find_encoder' failed");
			}

			if ((mux->ctx_codec_video = avcodec_alloc_context3(mux->codec_video)) == NULL) {
				ERR_EXIT("%s", "'avcodec_alloc_context3' failed");
			}

			if ( (out_stream = avformat_new_stream(mux->ctx_format, mux->codec_video)) == NULL) {
				ERR_EXIT("%s", "Could not create output stream");
			}

			INFO("%d -- %d", mux->ctx_codec_video->codec_id, mux->ctx_codec_video->codec_id);
			
			mux->ctx_codec_video->bit_rate     = mux_inp->ctx_codec_video->bit_rate;
			mux->ctx_codec_video->width        = mux_inp->ctx_codec_video->width;
			mux->ctx_codec_video->height       = mux_inp->ctx_codec_video->height;
			mux->ctx_codec_video->time_base.num    =  1; //mux_inp->ctx_codec_video->time_base.num;
			mux->ctx_codec_video->time_base.den    = 25; //mux_inp->ctx_codec_video->time_base.den;
			mux->ctx_codec_video->framerate.num    = 25; //mux_inp->ctx_codec_video->framerate.num;
			mux->ctx_codec_video->framerate.den    = 1; //mux_inp->ctx_codec_video->framerate.den;
			mux->ctx_codec_video->gop_size     = mux_inp->ctx_codec_video->gop_size;
			mux->ctx_codec_video->max_b_frames = mux_inp->ctx_codec_video->max_b_frames;
			mux->ctx_codec_video->pix_fmt      = mux_inp->ctx_codec_video->pix_fmt;
			mux->ctx_codec_video->frame_size      = mux_inp->ctx_codec_video->frame_size;

			if ((ret = av_opt_set(mux->ctx_codec_video->priv_data, "preset", "fast", 0)) < 0) {
				ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
			}

			/// Open output codec
			if ((ret = avcodec_open2(mux->ctx_codec_video, mux->codec_video, NULL)) < 0) {
				ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
			}


			// Open output
			if (!(mux->ctx_format->oformat->flags & AVFMT_NOFILE)) {
				if ( (ret = avio_open(&mux->ctx_format->pb, name, AVIO_FLAG_WRITE)) < 0) {
					ERR_EXIT("'%s' failed: %s", "avio_open", av_err2str(ret));
				}
			}

			//Write file header
			if ( (ret = avformat_write_header(mux->ctx_format, NULL)) < 0) {
				ERR_EXIT("'%s' failed: %s", "avformat_write_header", av_err2str(ret));
			}
			out_stream->codecpar->codec_tag = in_stream->codecpar->codec_tag;
			if (mux->ctx_format->oformat->flags & AVFMT_GLOBALHEADER)
				mux->ctx_codec_video->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}


	}



	INFO("%s" , "dump output format");
	av_dump_format(mux->ctx_format, 0, name, 1);

	return mux;
}

void
mux_out_free (muxer_t* mux)
{

}