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


static void muxer_add_video_stream(const char* name, int width, int height);
static int  muxer_add_subtitle(const char* text);
static void muxer_img_conv_init(void);
static int  muxer_prepare_frame(AVFrame *src);

static AVDictionary *opts;
static muxer_t *mux;
struct SwsContext *ctx_sws;
struct SwsContext *ctx_sws_sub;
static uint8_t * buf;
static uint8_t * buf_sub;
static AVFrame *dst;
static AVFrame *dst_sub;

muxer_t *
muxer_new (const char* name, enum AVPixelFormat pix_fmt, int width, int height) {
	mux = NULL;
	int ret = 0;

	if ((mux = (muxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	mux->fc      = 1;
	mux->width   = width;
	mux->height  = height;
	mux->pix_fmt_inp = pix_fmt;
	mux->pix_fmt_out = OUTPUT_PIXFMT;

	muxer_add_video_stream(name, width, height);
	muxer_img_conv_init();
	ass_init(width, height);

	if ( (ret = avformat_write_header(mux->ctx_f, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_write_header", av_err2str(ret));
	}

	av_dump_format(mux->ctx_f, 0, name, 1);
	return mux;
}

static void
muxer_add_video_stream(const char* name, int width, int height) {
	int ret = 0;

	if ((ret = avformat_alloc_output_context2(&mux->ctx_f, NULL, OUTPUT_FORMAT, name)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avformat_alloc_output_context2", av_err2str(ret));
	}

	if (mux->ctx_f == NULL) {
		ERR_EXIT("%s", "Could not create output context");
	}

	if ((mux->cv = avcodec_find_encoder(OUTPUT_VIDEO_ENCODER)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_find_encoder");
	}

	if ((mux->sv = avformat_new_stream(mux->ctx_f, mux->cv)) == NULL) {
		ERR_EXIT("'%s' failed", "avformat_new_stream");
	}

	mux->sv->codecpar->bit_rate   = OUTPUT_BITRATE;
	mux->sv->codecpar->profile    = OUTPUT_H264_PROFILE;
	mux->sv->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	mux->sv->codecpar->codec_id   = OUTPUT_VIDEO_ENCODER;

	if ((mux->ctx_cv = avcodec_alloc_context3(mux->cv)) == NULL) {
		ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_cv, mux->sv->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	mux->ctx_cv->time_base  = (AVRational){1, OUTPUT_FRAMERATE};
	mux->ctx_cv->gop_size   = OUTPUT_GOPSIZE;
	mux->ctx_cv->pix_fmt    = OUTPUT_PIXFMT;
	mux->ctx_cv->width      = width;
	mux->ctx_cv->height     = height;


	if ((ret = av_dict_set(&opts, "preset", OUTPUT_H264_PRESET, 0)) < 0) {
		ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
	}

	if ((ret = avcodec_open2(mux->ctx_cv, mux->cv, &opts)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}

	if (!(mux->ctx_f->oformat->flags & AVFMT_NOFILE)) {
		if ( (ret = avio_open(&mux->ctx_f->pb, name, AVIO_FLAG_WRITE)) < 0) {
			ERR_EXIT("'%s' failed: %s", "avio_open", av_err2str(ret));
		}
	}
}

void
muxer_free (muxer_t* mux) {
	av_free(buf);
	av_free(dst);
	av_dict_free(&opts);
	avcodec_close(mux->ctx_cv);
	avio_close(mux->ctx_f->pb);
	av_free(mux->ctx_f);
	sws_freeContext(ctx_sws);

}

/// image resizing/scaling

void muxer_img_conv_init(void) {
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

static int
muxer_prepare_frame(AVFrame *src) {
	int ret = 0;

	if (( ret = sws_scale(ctx_sws_sub, (const uint8_t * const*) src->data, src->linesize, 0, mux->height, dst_sub->data, dst_sub->linesize)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}
	/// now we have RGB24 frame

	dst_sub->format = AV_PIX_FMT_RGB24;
	dst_sub->width  = mux->width;
	dst_sub->height = ret;

	ASS_Image *sub = ass_get_track("A");
	blend(dst_sub, sub);

	if (( ret = sws_scale(ctx_sws, (const uint8_t * const*) dst_sub->data, dst_sub->linesize, 0, mux->height, dst->data, dst->linesize)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}

	dst->pts    = mux->fc++;
	dst->format = mux->pix_fmt_out;
	dst->width  = mux->width;
	dst->height = ret;

	return ret;
}

int
muxer_encode_frame(AVFrame *src, const char *sub) {

	int ret = 0;

	if ((ret = muxer_prepare_frame(src)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "muxer_prepare_frame", av_err2str(ret));
	}

	if ((ret = muxer_add_subtitle(sub)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "muxer_add_subtitle", av_err2str(ret));
	}

	if ((ret = avcodec_send_frame(mux->ctx_cv, dst)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
	}

	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;
	packet.side_data_elems = 0;

	while ((ret = avcodec_receive_packet(mux->ctx_cv, &packet)) >= 0) {

		//INFO("packet.size = %d 0x%h", packet.size, packet.size);
		//av_hex_dump_log(NULL, AV_LOG_INFO, packet.data, packet.size);

		if (src->key_frame)
			packet.flags |= AV_PKT_FLAG_KEY;
		packet.stream_index = mux->sv->index;
		av_packet_rescale_ts(&packet, mux->ctx_cv->time_base, mux->sv->time_base);

		if ((ret = av_interleaved_write_frame(mux->ctx_f, &packet)) < 0) {
			ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
		}
	}
	if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
	}

	return ret;
}

/// render subtitle on currnet frame

static int
muxer_add_subtitle(const char *text) {

	ass_get_track(text);
	return 0;
}
