#include <libavutil/imgutils.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avassert.h>
#include <libavutil/intreadwrite.h>
#include <libswscale/swscale.h>
#include <ass/ass.h>

#include "muxer.h"
#include "err.h"
#include "log.h"
#include "sub.h"


#define OUTPUT_FORMAT         "mpegts"
#define OUTPUT_PIXFMT         AV_PIX_FMT_YUV420P
#define OUTPUT_PIXFMT_SUB     AV_PIX_FMT_RGB24
#define OUTPUT_VIDEO_ENCODER  AV_CODEC_ID_H264
#define OUTPUT_CHANNELS       2
#define OUTPUT_H264_PROFILE   FF_PROFILE_H264_HIGH
#define OUTPUT_H264_PRESET    "fast"

#define OUTPUT_BITRATE        400 * 1000
#define OUTPUT_FRAMERATE      25
#define OUTPUT_GOPSIZE        15


static void muxer_add_video_stream();
static void muxer_add_audio_stream(AVCodecParameters *par);
static void muxer_img_conv_init(void);

static muxer_t      *mux;
static AVDictionary *opts;        // codec options
struct SwsContext   *ctx_sws;     // transcoding sws contex
struct SwsContext   *ctx_sws_sub; // transcoding sws contex for sub
static uint8_t      *buf;         // buffer for ctx_sws
static uint8_t      *buf_sub;     // buffer for ctx_sws_sub
static AVFrame      *vframe;      // video frame after decoding
static AVFrame      *vframe_sub;  // video frame with sub
static AVPacket     *packet;      // encoded data
static int key_frame;

static AVFormatContext   *ctx_f;  // format context
static AVCodecContext    *ctx_cv; // codec context video
static AVCodec           *cv;     // codec video
static AVStream          *sv;     // stream video
static int sia = -1; // stream index audio
static int siv = -1; // stream index video
static AVRational atb;
static AVRational vtb;
static AVStream          *sa;     // stream audio
//static AVCodec           *ca;     // codec audio
//static AVCodecContext    *ctx_ca; // codec context audio

muxer_t *
muxer_new (const char* name, demuxer_t *demux) {
	int ret = 0;

	if ((packet = av_packet_alloc()) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "av_packet_alloc");
	}

	if ((mux = (muxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "av_malloc");
	}

	if ((ret = avformat_alloc_output_context2(&ctx_f, NULL, OUTPUT_FORMAT, name)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "avformat_alloc_output_context2", av_err2str(ret));
	}

	if (ctx_f == NULL) {
		ERR_EXIT("MUXER:%s", "Could not create output context");
	}

	mux->fc     = 1;
	mux->width  = demux->width;
	mux->height = demux->height;
	mux->pix_fmt_inp = demux->pix_fmt;
	mux->pix_fmt_out = OUTPUT_PIXFMT;
	atb = demux->atb;
	vtb = demux->vtb;
	muxer_add_video_stream();
	if (demux->audio)
		muxer_add_audio_stream(demux->pa);
	muxer_img_conv_init();
	sub_init(mux->width, mux->height);

	if ( (ret = avformat_write_header(ctx_f, NULL)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "avformat_write_header", av_err2str(ret));
	}

	if (!(ctx_f->oformat->flags & AVFMT_NOFILE)) {
		if ( (ret = avio_open(&ctx_f->pb, name, AVIO_FLAG_WRITE)) < 0) {
			ERR_EXIT("MUXER:'%s' failed: %s", "avio_open", av_err2str(ret));
		}
	}

	av_dump_format(ctx_f, 0, name, 1);
	return mux;
}

static void
muxer_add_audio_stream(AVCodecParameters *par) {
	int ret = 0;

	if ((sa = avformat_new_stream(ctx_f, NULL)) == NULL) {
		ERR_EXIT("%s", "Could not create audio stream");
	}
	sia = sa->index;
	if ((ret = avcodec_parameters_copy(sa->codecpar, par)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_copy", av_err2str(ret));
	}
}

static void
muxer_add_video_stream() {
	int ret = 0;

	if ((cv = avcodec_find_encoder(OUTPUT_VIDEO_ENCODER)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "avcodec_find_encoder");
	}

	if ((sv = avformat_new_stream(ctx_f, cv)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "avformat_new_stream");
	}
	siv = sv->index;
	sv->codecpar->bit_rate   = OUTPUT_BITRATE;
	sv->codecpar->profile    = OUTPUT_H264_PROFILE;
	sv->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	sv->codecpar->codec_id   = OUTPUT_VIDEO_ENCODER;
	sv->codecpar->width      = mux->width;
	sv->codecpar->height     = mux->height;


	if ((ctx_cv = avcodec_alloc_context3(cv)) == NULL) {
		ERR_EXIT("MUXER:%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(ctx_cv, sv->codecpar)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	ctx_cv->time_base  = (AVRational){1, OUTPUT_FRAMERATE};
	ctx_cv->gop_size   = OUTPUT_GOPSIZE;
	ctx_cv->pix_fmt    = OUTPUT_PIXFMT;
	ctx_cv->width      = mux->width;
	ctx_cv->height     = mux->height;

	if ((ret = av_dict_set(&opts, "preset", OUTPUT_H264_PRESET, 0)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "av_opt_set", av_err2str(ret));
	}

	if ((ret = avcodec_open2(ctx_cv, cv, &opts)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}
}

void
muxer_free (void) {
	av_packet_unref(packet);
	av_free(buf);
	av_frame_free(&vframe);
	av_free(buf_sub);
	av_frame_free(&vframe_sub);
	av_dict_free(&opts);
	avcodec_close(ctx_cv);
	avio_close(ctx_f->pb);
	av_free(ctx_f);
	sws_freeContext(ctx_sws);
	sws_freeContext(ctx_sws_sub);
}

/// image resizing/scaling

void
muxer_img_conv_init(void) {
	int size = 0, size_sub = 0, ret = 0;

	if ((vframe = av_frame_alloc()) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "av_frame_alloc");
	}

	if ((vframe_sub = av_frame_alloc()) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "av_frame_alloc");
	}

	if ((size = av_image_get_buffer_size(mux->pix_fmt_out, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("MUXER:'%s' failed", "av_image_get_buffer_size");
	}

	if ((size_sub = av_image_get_buffer_size(OUTPUT_PIXFMT_SUB, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("MUXER:'%s' failed", "av_image_get_buffer_size");
	}

	if ((buf = (uint8_t*) av_mallocz(size)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "av_frame_alloc");
	}

	if ((buf_sub = (uint8_t*) av_mallocz(size_sub)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed", "av_frame_alloc");
	}

	if ((ret = av_image_fill_arrays(vframe->data, vframe->linesize, buf, mux->pix_fmt_out, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	if ((ret = av_image_fill_arrays(vframe_sub->data, vframe_sub->linesize, buf_sub, OUTPUT_PIXFMT_SUB, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	if ((ctx_sws = sws_getContext(
								mux->width,
								mux->height,
								OUTPUT_PIXFMT_SUB,
								mux->width,
								mux->height,
								mux->pix_fmt_out,
								SWS_BICUBIC, NULL, NULL, NULL)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed: %s", "sws_getContext", av_err2str(ret));
	}

	if ((ctx_sws_sub = sws_getContext(
									mux->width,
									mux->height,
									mux->pix_fmt_inp,
									mux->width,
									mux->height,
									OUTPUT_PIXFMT_SUB,
									SWS_BICUBIC, NULL, NULL, NULL)) == NULL) {
		ERR_EXIT("MUXER:'%s' failed: %s", "sws_getContext", av_err2str(ret));
	}
}

int
muxer_pack_video(AVFrame *src, const char* subtitle) {
	int ret = 0;
	key_frame = src->key_frame;

	if (( ret = sws_scale(ctx_sws_sub, (const uint8_t * const*) src->data, src->linesize, 0, mux->height, vframe_sub->data, vframe_sub->linesize)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}
	/// now we have RGB24 frame in dst_sub

	vframe_sub->format = AV_PIX_FMT_RGB24;
	vframe_sub->width  = mux->width;
	vframe_sub->height = ret;

	sub_draw(vframe_sub, subtitle);

	if (( ret = sws_scale(ctx_sws, (const uint8_t * const*) vframe_sub->data, vframe_sub->linesize, 0, mux->height, vframe->data, vframe->linesize)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}

	vframe->pts    = mux->fc++;
	vframe->format = mux->pix_fmt_out;
	vframe->width  = mux->width;
	vframe->height = ret;

	if ((ret = avcodec_send_frame(ctx_cv, vframe)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
	}

	return ret;
}

int
muxer_encode_video(void) {
	return avcodec_receive_packet(ctx_cv, packet);
}

void
muxer_write_audio(AVPacket *p) {
	if (sia == -1)
		return;
	int ret = 0;
	p->stream_index = sia;
	av_packet_rescale_ts(p, atb, sa->time_base);	
	if ((ret = av_interleaved_write_frame(ctx_f, p)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}
}

void
muxer_write_video(void) {

	//INFO("packet.size = %d", packet->size);
	//av_hex_dump_log(NULL, AV_LOG_INFO, packet.data, packet.size);
	int ret = 0;
	if (key_frame)
		packet->flags |= AV_PKT_FLAG_KEY;

	packet->stream_index = siv;

	av_packet_rescale_ts(packet, ctx_cv->time_base, sv->time_base);

	if ((ret = av_interleaved_write_frame(ctx_f, packet)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}
}

void
muxer_finish(void) {
	int ret = 0;
	if ((ret = av_write_trailer(ctx_f)) < 0) {
		ERR_EXIT("MUXER:'%s' failed: %s", "av_write_trailer", av_err2str(ret));
	}
}