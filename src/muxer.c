#include <libavutil/imgutils.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avassert.h>
#include <libavutil/intreadwrite.h>
#include <libswscale/swscale.h>
#include "libavutil/pixfmt.h"

#include "muxer.h"
#include "err.h"
#include "log.h"
#include "utils.h"
#include "ass.h"


#define OUTPUT_FORMAT            "mpegts"
#define OUTPUT_PIXFMT             AV_PIX_FMT_YUV420P
#define OUTPUT_VIDEO_ENCODER      AV_CODEC_ID_H264
#define OUTPUT_SUB_BITMAP_ENCODER AV_CODEC_ID_DVB_SUBTITLE
#define OUTPUT_H264_PROFILE       FF_PROFILE_H264_HIGH
#define OUTPUT_H264_PRESET        "fast"

#define OUTPUT_BITRATE            400 * 1000
#define OUTPUT_FRAMERATE          25
#define OUTPUT_GOPSIZE            15

extern const uint8_t avpriv_cga_font[2048];

static void muxer_add_video_stream(const char* name, int width, int height);
static void muxer_add_subdvb_stream(int width, int height);
static void muxer_add_out_frame(void);

static AVDictionary *opts;
static muxer_t *mux;
struct SwsContext *ctx_sws;
uint8_t * buf;
AVFrame *dst;

static int sub_dvb_init = 0;

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
	muxer_add_subdvb_stream(width, height);
	muxer_add_out_frame();

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

static void
muxer_add_subdvb_stream(int width, int height) {
	if (sub_dvb_init > 0)
		return;

	int ret = 0;

	if ((mux->cb = avcodec_find_encoder(OUTPUT_SUB_BITMAP_ENCODER)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_find_encoder");
	}

	if ((mux->sb = avformat_new_stream(mux->ctx_f, mux->cb)) == NULL) {
		ERR_EXIT("'%s' failed", "avformat_new_stream");
	}

	mux->sb->codecpar->codec_id   = OUTPUT_SUB_BITMAP_ENCODER;
	mux->sb->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;

	if ((mux->ctx_cb = avcodec_alloc_context3(mux->cb)) == NULL) {
		ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_cb, mux->sb->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	mux->ctx_cb->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	mux->ctx_cb->time_base  = mux->ctx_cv->time_base;

	if ((ret = avcodec_open2(mux->ctx_cb, mux->cb, NULL)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}

	sub_dvb_init = 0;
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

void muxer_add_out_frame(void) {
	int size = 0, ret = 0;


	if ((dst = av_frame_alloc()) == NULL) {
		ERR_EXIT("VIDEO:'%s' failed", "av_frame_alloc");
	}

	if ((size = av_image_get_buffer_size(mux->pix_fmt_out, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed", "av_image_get_buffer_size");
	}

	if ((buf = (uint8_t*) av_mallocz(size)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
	}

	if ((ret = av_image_fill_arrays(dst->data, dst->linesize, buf, mux->pix_fmt_out, mux->width, mux->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	if ((ctx_sws = sws_getContext(
								mux->width,
								mux->height,
								mux->pix_fmt_inp,
								mux->width,
								mux->height,
								mux->pix_fmt_out,
								SWS_BICUBIC, NULL, NULL, NULL)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "sws_getContext", av_err2str(ret));
	}

}

int
muxer_encode_frame(AVFrame *src) {

	int ret = 0, result = -1;

	if (( ret = sws_scale(ctx_sws, (const uint8_t * const*) src->data, src->linesize, 0, mux->height, dst->data, dst->linesize)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "sws_scale", av_err2str(ret));
	}


	dst->pts    = mux->fc++;
	dst->format = mux->pix_fmt_out;
	dst->width  = mux->width;
	dst->height = ret;

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
		//packet.pos = -1;		
		av_packet_rescale_ts(&packet, mux->ctx_cv->time_base, mux->sv->time_base);

		if ((ret = av_interleaved_write_frame(mux->ctx_f, &packet)) < 0) {
			ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
		}
		result = ret;
	}
	if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
	}



	return result;
}

static int
add_rect (AVSubtitle *sub, const char *text) {

	int size = 0, ret = 0, h = 100;
	AVSubtitleRect *rect;
	uint8_t * buf[4] = {0};
	//int linesize[4] = {0};

	sub->num_rects = 1;

	if ((size = av_image_get_buffer_size(mux->pix_fmt_out, mux->width, h, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed", "av_image_get_buffer_size");
	}

	if ((sub->rects = (AVSubtitleRect**) av_mallocz(sizeof(*sub->rects))) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_mallocz");
	}

	if ((rect = (AVSubtitleRect*) av_mallocz(sizeof(*sub->rects[0]))) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_mallocz");
	}

	sub->rects[0] = rect;

	//	if ((sub->rects[0]->data[0] = (uint8_t *) av_mallocz(size)) == NULL) {
	//		ERR_EXIT("IMAGE:'%s' failed", "av_mallocz");
	//	}
	//	
	//	if ((sub->rects[0]->data[1] = (uint8_t *) av_mallocz(size)) == NULL) {
	//		ERR_EXIT("IMAGE:'%s' failed", "av_mallocz");
	//	}



	//
	if ((buf[0] = (uint8_t*) av_mallocz(size)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
	}
	//
	//	if ((buf[1] = (uint8_t*) av_mallocz(size)) == NULL) {
	//		ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
	//	}
	//	
	if ((ret = av_image_fill_arrays(rect->data, rect->linesize, buf[0], mux->pix_fmt_out, mux->width, h, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	ptrdiff_t linesize[4] = { rect->linesize[0], 0, 0, 0 };

	if ((ret = av_image_fill_black(rect->data, linesize, mux->pix_fmt_out, AVCOL_RANGE_JPEG, mux->width, h)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	//	const uint8_t *font;
	//	int font_height;
	//	int i;
	//	int x = 0, y = 100;
	//	uint32_t color = 0;
	//	font = avpriv_cga_font, font_height = 8;
	//
	//	char *txt = "I am StackOverflow amused.";
	//	for (i = 0; txt[i]; i++) {
	//		int char_y, mask;
	//
	//		uint8_t *p = buf[0] + y * linesize[0] + (x + i * 8) * 4;
	//		for (char_y = 0; char_y < font_height; char_y++) {
	//			for (mask = 0x80; mask; mask >>= 1) {
	//				if (font[txt[i] * font_height + char_y] & mask)
	//					AV_WL32(p, color);
	//				p += 4;
	//			}
	//			p += linesize[0] - 8 * 4;
	//		}
	//	}
	//	av_image_copy(rect->data, rect->linesize, (const uint8_t**) buf, linesize, mux->pix_fmt, mux->width, h);
	rect->type = SUBTITLE_BITMAP;
	rect->w    = mux->width;
	rect->h    = h;
	rect->x    = 100;
	rect->y    = 100;


	return sub->num_rects;
}

int
muxer_encode_subtitle(char *text) {

	int buf_max_size = 1024 * 1024, buf_size = 0, ret = 0;
	OK
	AVPacket packet;
	uint8_t * buf;
	AVSubtitle sub;
	sub.num_rects = 0;
	//sub.pts = out->fc;
	sub.start_display_time = 0;
	add_rect(&sub, text);

	if ((buf = av_mallocz(buf_max_size)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_mallocz");
	}

	if ( (buf_size = avcodec_encode_subtitle(mux->ctx_cb, buf, buf_max_size, &sub)) < 0) {
		ERR_EXIT("SUBTITLE:'%s' failed: %s", "avcodec_encode_subtitle", av_err2str(buf_size));
	}
	av_hex_dump_log(NULL, AV_LOG_INFO, buf, buf_size);
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	packet.stream_index = mux->sb->index;
	packet.pos = mux->fc;
	packet.pts = mux->fc * 1000;
	packet.dts = mux->fc * 1000;
	packet.duration = 1000000;
	log_packet(mux->ctx_f, &packet);

	if (packet.pts != AV_NOPTS_VALUE)
		packet.pts = av_rescale_q(packet.pts, mux->ctx_cb->time_base, mux->sb->time_base);

	if (packet.dts != AV_NOPTS_VALUE)
		packet.dts = av_rescale_q(packet.dts, mux->ctx_cb->time_base, mux->sb->time_base);

	if ((ret = av_interleaved_write_frame(mux->ctx_f, &packet)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}

	av_free(buf);

	//avsubtitle_free(&sub);

	return 0;
}