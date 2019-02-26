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
#include "utils.h"
#include "ass.h"


#define OUTPUT_FORMAT            "mpegts"
#define OUTPUT_PIXFMT             AV_PIX_FMT_YUV420P
#define OUTPUT_PIXFMT_SUB         AV_PIX_FMT_RGB24
#define OUTPUT_VIDEO_ENCODER      AV_CODEC_ID_H264
#define OUTPUT_SUB_BITMAP_ENCODER AV_CODEC_ID_DVB_SUBTITLE
#define OUTPUT_H264_PROFILE       FF_PROFILE_H264_HIGH
#define OUTPUT_H264_PRESET        "fast"

#define OUTPUT_BITRATE            400 * 1000
#define OUTPUT_FRAMERATE          25
#define OUTPUT_GOPSIZE            15


static void muxer_add_video_stream(AVCodecParameters *param);
static void muxer_add_audio_stream(AVCodecParameters *param);
static void muxer_img_conv_init(void);

static muxer_t      *mux;
static AVDictionary *opts;        // codec options
struct SwsContext   *ctx_sws;     // transcoding sws contex
struct SwsContext   *ctx_sws_sub; // transcoding sws contex for sub
static uint8_t      *buf;         // buffer for ctx_sws
static uint8_t      *buf_sub;     // buffer for ctx_sws_sub
static AVFrame      *dst;         // frame after decoding
static AVFrame      *dst_sub;     // frame with sub
static AVPacket     *packet;      // encoded data
static int key_frame;

static AVFormatContext   *ctx_f;  // format context
static AVCodecContext    *ctx_cv; // codec context video
static AVCodec           *cv;     // codec video
static AVStream          *sv;     // stream video
static int sia; // stream index audio
static int siv; // stream index video
static AVStream          *sa;     // stream audio
//static AVCodec           *ca;     // codec audio
//static AVCodecContext    *ctx_ca; // codec context audio

muxer_t *
muxer_new (const char* name, demuxer_t *demux) {
	int ret = 0;

	if ((packet = av_packet_alloc()) == NULL) {
		ERR_EXIT("VIDEO:'%s' failed", "av_pzcket_alloc");
	}

	if ((mux = (muxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ((ret = avformat_alloc_output_context2(&ctx_f, NULL, OUTPUT_FORMAT, name)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_alloc_output_context2", av_err2str(ret));
	}

	if (ctx_f == NULL) {
		ERR_EXIT("%s", "Could not create output context");
	}

	mux->fc      = 1;
	mux->width   = demux->width;
	mux->height  = demux->height;
	mux->pix_fmt_inp = demux->pix_fmt;
	mux->pix_fmt_out = OUTPUT_PIXFMT;

	muxer_add_video_stream(demux->pv);
	muxer_add_audio_stream(demux->pa);		
	muxer_img_conv_init();
	ass_init(mux->width, mux->height);

	if ( (ret = avformat_write_header(ctx_f, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_write_header", av_err2str(ret));
	}

	if (!(ctx_f->oformat->flags & AVFMT_NOFILE)) {
		if ( (ret = avio_open(&ctx_f->pb, name, AVIO_FLAG_WRITE)) < 0) {
			ERR_EXIT("'%s' failed: %s", "avio_open", av_err2str(ret));
		}
	}

	av_dump_format(ctx_f, 0, name, 1);
	return mux;
}
/// simple stream without transcoding

static void
muxer_add_audio_stream(AVCodecParameters *param) {
	int ret = 0;

	if ((sa = avformat_new_stream(ctx_f, NULL)) == NULL) {
		ERR_EXIT("%s", "Could not create audio stream");
	}
	sia = sa->index;
	if ( (ret = avcodec_parameters_copy(sa->codecpar, param)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_copy", av_err2str(ret));
	}
}

static void
muxer_add_video_stream(AVCodecParameters *param) {
	int ret = 0;

	if ((cv = avcodec_find_encoder(OUTPUT_VIDEO_ENCODER)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_find_encoder");
	}

	if ((sv = avformat_new_stream(ctx_f, cv)) == NULL) {
		ERR_EXIT("'%s' failed", "avformat_new_stream");
	}
	siv = sv->index;
	sv->codecpar->bit_rate   = OUTPUT_BITRATE;
	sv->codecpar->profile    = OUTPUT_H264_PROFILE;
	sv->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	sv->codecpar->codec_id   = OUTPUT_VIDEO_ENCODER;

	if ((ctx_cv = avcodec_alloc_context3(cv)) == NULL) {
		ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(ctx_cv, sv->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	ctx_cv->time_base  = (AVRational){1, OUTPUT_FRAMERATE};
	ctx_cv->gop_size   = OUTPUT_GOPSIZE;
	ctx_cv->pix_fmt    = OUTPUT_PIXFMT;
	ctx_cv->width      = mux->width;
	ctx_cv->height     = mux->height;


	if ((ret = av_dict_set(&opts, "preset", OUTPUT_H264_PRESET, 0)) < 0) {
		ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
	}

	if ((ret = avcodec_open2(ctx_cv, cv, &opts)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}
}

void
muxer_free (void) {
	av_packet_unref(packet);
	av_free(buf);
	av_free(dst);
	av_free(buf_sub);
	av_free(dst_sub);
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

	if ((dst = av_frame_alloc()) == NULL) {
		ERR_EXIT("VIDEO:'%s' failed", "av_frame_alloc");
	}

	if ((dst_sub = av_frame_alloc()) == NULL) {
		ERR_EXIT("VIDEO:'%s' failed", "av_frame_alloc");
	}

	if ((size = av_image_get_buffer_size(mux->pix_fmt_out, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed", "av_image_get_buffer_size");
	}

	if ((size_sub = av_image_get_buffer_size(OUTPUT_PIXFMT_SUB, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed", "av_image_get_buffer_size");
	}

	if ((buf = (uint8_t*) av_mallocz(size)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
	}

	if ((buf_sub = (uint8_t*) av_mallocz(size_sub)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
	}

	if ((ret = av_image_fill_arrays(dst->data, dst->linesize, buf, mux->pix_fmt_out, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	if ((ret = av_image_fill_arrays(dst_sub->data, dst_sub->linesize, buf_sub, OUTPUT_PIXFMT_SUB, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	if ((ctx_sws = sws_getContext(
		mux->width,
		mux->height,
		OUTPUT_PIXFMT_SUB,
		mux->width,
		mux->height,
		mux->pix_fmt_out,
		SWS_BICUBIC, NULL, NULL, NULL)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "sws_getContext", av_err2str(ret));
	}

	if ((ctx_sws_sub = sws_getContext(
		mux->width,
		mux->height,
		mux->pix_fmt_inp,
		mux->width,
		mux->height,
		OUTPUT_PIXFMT_SUB,
		SWS_BICUBIC, NULL, NULL, NULL)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "sws_getContext", av_err2str(ret));
	}
}

static void blend_single(AVFrame * frame, ASS_Image *img) {

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

	int x, y;
	unsigned char opacity = 255 - _a(img->color);
	unsigned char r = _r(img->color);
	unsigned char g = _g(img->color);
	unsigned char b = _b(img->color);

	unsigned char *src;
	unsigned char *dst;

	src = img->bitmap;
	dst = frame->data[0] + img->dst_y * frame->linesize[0] + img->dst_x * 3;
	for (y = 0; y < img->h; ++y) {
		for (x = 0; x < img->w; ++x) {
			unsigned k = ((unsigned) src[x]) * opacity / 255;
			// possible endianness problems
			dst[x * 3] = (k * b + (255 - k) * dst[x * 3]) / 255;
			dst[x * 3 + 1] = (k * g + (255 - k) * dst[x * 3 + 1]) / 255;
			dst[x * 3 + 2] = (k * r + (255 - k) * dst[x * 3 + 2]) / 255;
		}
		src += img->stride;
		dst += frame->linesize[0];
	}
}

static void blend(AVFrame *frame, ASS_Image *img) {
	int cnt = 0;
	while (img) {
		blend_single(frame, img);
		++cnt;
		img = img->next;
	}
	printf("%d images blended\n", cnt);
}
// convert image to output format

int
muxer_pack_video(AVFrame *src) {
	int ret = 0;
	key_frame = src->key_frame;

	if (( ret = sws_scale(ctx_sws_sub, (const uint8_t * const*) src->data, src->linesize, 0, mux->height, dst_sub->data, dst_sub->linesize)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}
	/// now we have RGB24 frame in dst_sub

	dst_sub->format = AV_PIX_FMT_RGB24;
	dst_sub->width  = mux->width;
	dst_sub->height = ret;

	//ASS_Image *sub = ass_get_track("A");
	//blend(dst_sub, sub);

	if (( ret = sws_scale(ctx_sws, (const uint8_t * const*) dst_sub->data, dst_sub->linesize, 0, mux->height, dst->data, dst->linesize)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}

	dst->pts    = mux->fc++;
	dst->format = mux->pix_fmt_out;
	dst->width  = mux->width;
	dst->height = ret;

	if ((ret = avcodec_send_frame(ctx_cv, dst)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
	}

	return ret;
}

int
muxer_encode_video(void) {
	return avcodec_receive_packet(ctx_cv, packet);
}

void
muxer_write_audio_packet(AVPacket *p) {	
	int ret = 0;
	p->stream_index = sia;
	av_packet_rescale_ts(p, ctx_cv->time_base, sv->time_base);
	if ((ret = av_interleaved_write_frame(ctx_f, p)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}
}

void
muxer_write_video(void) {

	INFO("packet.size = %d", packet->size);
	//av_hex_dump_log(NULL, AV_LOG_INFO, packet.data, packet.size);
	int ret = 0;
	if (key_frame)
		packet->flags |= AV_PKT_FLAG_KEY;

	packet->stream_index = siv;

	av_packet_rescale_ts(packet, ctx_cv->time_base, sv->time_base);

	if ((ret = av_interleaved_write_frame(ctx_f, packet)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}
}

void
muxer_finish(void) {
	av_write_trailer(ctx_f);
}