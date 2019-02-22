#include <libavutil/opt.h>

#include "mux_out.h"
#include "err.h"
#include "log.h"

muxer_t*
mux_out_new (const char* name, const char *format, enum AVCodecID video_codec_id, muxer_t *mux_inp) {
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
	int n = 0;
	for (int i = 0; i < mux_inp->ctx_format->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream
				*in_stream  = NULL;

		in_stream  = mux_inp->ctx_format->streams[i];
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			mux->index_audio = n++;
			if ( (mux->stream_audio = avformat_new_stream(mux->ctx_format, NULL)) == NULL) {
				ERR_EXIT("%s", "Could not create output stream");
			}
			//Copy the settings of AVCodecContext
			if ( (ret = avcodec_parameters_copy(mux->stream_audio->codecpar, in_stream->codecpar)) < 0) {
				ERR_EXIT("'%s' failed: %s", "avcodec_parameters_copy", av_err2str(ret));
			}

		}
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			mux->index_video = n++;
			if ((mux->codec_video = avcodec_find_encoder(video_codec_id)) == NULL) {
				ERR_EXIT("%s", "'avcodec_find_encoder' failed");
			}



			if ( (mux->stream_video = avformat_new_stream(mux->ctx_format, mux->codec_video)) == NULL) {
				ERR_EXIT("%s", "Could not create output stream");
				&opts
			}

			mux->ctx_codec_video = mux->stream_video->codec;

			if ((ret = avcodec_get_context_defaults3(mux->ctx_codec_video, &mux->codec_video)) < 0) {
				ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
			}

			//out_stream->sample_aspect_ratio = (AVRational){24, 1};
			//out_stream->r_frame_rate = (AVRational){24, 1};
			//out_stream->time_base = (AVRational){1, 600};
			//out_stream->codec->time_base = (AVRational){1, 600};
			//out_stream->avg_frame_rate = (AVRational){24, 1};
			//out_stream->disposition = AV_DISPOSITION_DEFAULT;

			mux->ctx_codec_video->codec_id = video_codec_id;
			mux->ctx_codec_video->bit_rate = 150 * 1000;
			/* Resolution must be a multiple of two. */
			mux->ctx_codec_video->width = mux_inp->ctx_codec_video->width;
			;
			mux->ctx_codec_video->height = mux_inp->ctx_codec_video->height;
			/* timebase: This is the fundamental unit of time (in seconds) in terms
			 * of which frame timestamps are represented. For fixed-fps content,
			 * timebase should be 1/framerate and timestamp increments should be
			 * identical to 1. */
			mux->ctx_codec_video->time_base.den = 24;
			mux->ctx_codec_video->time_base.num = 1;
			mux->ctx_codec_video->gop_size = 12; /* emit one intra frame every twelve frames at most */
			mux->ctx_codec_video->pix_fmt = AV_PIX_FMT_YUV420P;

			//if ((ret = avcodec_parameters_to_context(mux->ctx_codec_video, mux->stream_video->codecpar)) < 0) {
			//	ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
			//}
			//mux->ctx_codec_video->thread_count = 4;
			//			mux->ctx_codec_video->bit_rate     = 512 << 10; //mux_inp->ctx_codec_video->bit_rate;
			//			mux->ctx_codec_video->width        = mux_inp->ctx_codec_video->width;
			//			mux->ctx_codec_video->height       = mux_inp->ctx_codec_video->height;
			//			mux->ctx_codec_video->time_base = (AVRational){1, 600}; //    =  1; //mux_inp->ctx_codec_video->time_base.num;
			//			//mux->ctx_codec_video->time_base.den    = 25; //mux_inp->ctx_codec_video->time_base.den;
			//			//mux->ctx_codec_video->framerate = (AVRational){24, 1}; //.num    = mux_inp->ctx_codec_video->framerate.num;
			//			//mux->ctx_codec_video->framerate.den    = mux_inp->ctx_codec_video->framerate.den;
			//			mux->ctx_codec_video->gop_size     = 25; //mux_inp->ctx_codec_video->gop_size;
			//			mux->ctx_codec_video->max_b_frames = 0; //mux_inp->ctx_codec_video->max_b_frames;
			//			mux->ctx_codec_video->pix_fmt      = AV_PIX_FMT_YUV420P; //mux_inp->ctx_codec_video->pix_fmt;
			//			mux->ctx_codec_video->frame_size      = mux_inp->ctx_codec_video->frame_size;
			//			mux->ctx_codec_video->profile = FF_PROFILE_H264_MAIN;
			//			mux->ctx_codec_video->level = 31;
			//			mux->ctx_codec_video->qmax = 2;
			//			mux->ctx_codec_video->qmin = 32;
			//			mux->ctx_codec_video->delay = 0;
			//			mux->ctx_codec_video->keyint_min = 24;




			//			if ((ret = av_opt_set(mux->ctx_codec_video->priv_data, "preset", "slow", 0)) < 0) {
			//				ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
			//			}
			//
			//			if ((ret = av_opt_set(mux->ctx_codec_video->priv_data, "crf", "12", 0)) < 0) {
			//				ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
			//			}
			//
			//			if ((ret = av_opt_set(mux->ctx_codec_video->priv_data, "b-pyramid", "0", 0)) < 0) {
			//				ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
		}

		//if ((ret = av_opt_set(mux->ctx_codec_video->priv_data, "profile", "baseline", 0)) < 0) {
		//	ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
		//}

		/// Open output codec

		//mux->stream_video->codecpar->codec_tag = 0;
		//mux->ctx_format->oformat->flags |= AVFMT_FLAG_GENPTS;

		if (mux->ctx_format->oformat->flags & AVFMT_GLOBALHEADER)
			mux->ctx_codec_video->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		if ((ret = avcodec_open2(mux->ctx_codec_video, mux->codec_video, NULL)) < 0) {
			ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
		}



		//if ((ret = av_opt_set(mux->ctx_format, "use_wallclock_as_timestamps", "1", 0)) < 0) {
		//	ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
		//}
		// Open output
		if (!(mux->ctx_format->oformat->flags & AVFMT_NOFILE)) {
			if ( (ret = avio_open(&mux->ctx_format->pb, name, AVIO_FLAG_WRITE)) < 0) {
				ERR_EXIT("'%s' failed: %s", "avio_open", av_err2str(ret));
			}
		}

		// https://gist.github.com/sdumetz/961585ea70f82e4fb27aadf66b2c9cb2
		//			AVDictionary *fmt_opts = NULL;
		//
		//			if ((ret = av_dict_set(&fmt_opts, "movflags", "faststart", 0)) < 0) {
		//				ERR_EXIT("'%s' failed: %s", "av_dict_set movflags", av_err2str(ret));
		//			}
		//			//default brand is "isom", which fails on some devices
		//
		//			if ((ret = av_dict_set(&fmt_opts, "brand", "mp42", 0)) < 0) {
		//				ERR_EXIT("'%s' failed: %s", "av_dict_set brand", av_err2str(ret));
		//			}

		INFO("%s" , "dump output format");
		av_dump_format(mux->ctx_format, 0, name, 1);

		//Write file header
		if ( (ret = avformat_write_header(mux->ctx_format, NULL)) < 0) {
			ERR_EXIT("'%s' failed: %s", "avformat_write_header", av_err2str(ret));
		}

	}








	return mux;
}

void
mux_out_free (muxer_t* mux) {

}