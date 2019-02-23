#include <libavutil/imgutils.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avassert.h>

#include "muxer.h"
#include "err.h"
#include "log.h"
#include "utils.h"
#include "ass.h"

#define OUTPUT_FORMAT           "mpegts"
#define OUTPUT_PIXFMT           AV_PIX_FMT_YUV420P
#define OUTPUT_VIDEO_ENCODER    AV_CODEC_ID_H264
#define OUTPUT_SUBTITLE_ENCODER AV_CODEC_ID_MOV_TEXT
#define OUTPUT_BITRATE    150 * 1000
#define OUTPUT_FRAMERATE  30
#define OUTPUT_GOPSIZE    12


static AVDictionary *opts;

muxer_t *
muxer_new (const char* name, int width, int height)
{
	muxer_t *mux = NULL;
	int ret = 0;

	if ((mux = (muxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	mux->fc      = 1;
	mux->width   = width;
	mux->height  = height;
	mux->pix_fmt = OUTPUT_PIXFMT;

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

	if ((mux->ctx_cv = avcodec_alloc_context3(mux->cv)) == NULL) {
		ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_cv, mux->sv->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	mux->ctx_cv->bit_rate   = OUTPUT_BITRATE;
	mux->ctx_cv->width      = width;
	mux->ctx_cv->height     = height;
	mux->ctx_cv->time_base  = (AVRational){1, OUTPUT_FRAMERATE};
	mux->ctx_cv->gop_size   = OUTPUT_GOPSIZE;
	mux->ctx_cv->pix_fmt    = OUTPUT_PIXFMT;
	mux->ctx_cv->profile    = FF_PROFILE_H264_BASELINE;

	if ((ret = av_dict_set(&opts, "preset", "slow", 0)) < 0) {
		ERR_EXIT("'%s' failed: %s", "av_opt_set", av_err2str(ret));
	}

	if ((ret = avcodec_open2(mux->ctx_cv, mux->cv, &opts)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}

	// subtitle str
	if ((mux->cs = avcodec_find_encoder(OUTPUT_SUBTITLE_ENCODER)) == NULL) {
		ERR_EXIT("'%s' failed", "avcodec_find_encoder");
	}

	if ((mux->ss = avformat_new_stream(mux->ctx_f, mux->cs)) == NULL) {
		ERR_EXIT("'%s' failed", "avformat_new_stream");
	}

	mux->ss->codecpar->codec_id   = OUTPUT_SUBTITLE_ENCODER;
	mux->ss->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;

	if ((mux->ctx_cs = avcodec_alloc_context3(mux->cs)) == NULL) {
		ERR_EXIT("%s", "'avcodec_get_context_defaults3' failed");
	}

	if ((ret = avcodec_parameters_to_context(mux->ctx_cs, mux->ss->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	mux->ctx_cs->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	mux->ctx_cs->time_base  = mux->ctx_cv->time_base;

	if ((ret = ass_subtitle_header(
		mux->ctx_cs,
		width, height,
		ASS_DEFAULT_FONT,
		ASS_DEFAULT_FONT_SIZE,
		ASS_DEFAULT_COLOR,
		ASS_DEFAULT_BACK_COLOR,
		ASS_DEFAULT_BOLD,
		ASS_DEFAULT_ITALIC,
		ASS_DEFAULT_UNDERLINE,
		ASS_DEFAULT_BORDERSTYLE,
		ASS_DEFAULT_ALIGNMENT)) < 0) {
		ERR_EXIT("'%s' failed: %s", "ff_ass_subtitle_header_default", av_err2str(ret));
	}

	if ((ret = avcodec_open2(mux->ctx_cs, mux->cs, NULL)) < 0) {
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
muxer_free (muxer_t* mux)
{
	av_dict_free(&opts);
	avcodec_close(mux->ctx_cv);
	avio_close(mux->ctx_f->pb);
	av_free(mux->ctx_f);
}

int
muxer_encode_frame(muxer_t *out, AVFrame *src)
{
	AVFrame* dst = NULL;
	int size = 0, ret = 0, result = -1;
	uint8_t * buf = NULL;

	if ((dst = av_frame_alloc()) == NULL) {
		ERR_EXIT("VIDEO:'%s' failed", "av_frame_alloc");
	}

	if ((size = av_image_get_buffer_size(out->pix_fmt, out->width, out->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed", "av_image_get_buffer_size");
	}

	if ((buf = (uint8_t*) av_mallocz(size)) == NULL) {
		ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
	}

	if ((ret = av_image_fill_arrays(dst->data, dst->linesize, buf, out->pix_fmt, out->width, out->height, 1)) < 0) {
		ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	}

	av_image_copy(dst->data, dst->linesize, (const uint8_t**) src->data, src->linesize, out->pix_fmt, out->width, out->height);

	dst->pts    = out->fc++;
	dst->format = out->pix_fmt;
	dst->width  = out->width;
	dst->height = out->height;

	if ((ret = avcodec_send_frame(out->ctx_cv, dst)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
	}

	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;
	packet.side_data_elems = 0;

	while ((ret = avcodec_receive_packet(out->ctx_cv, &packet)) >= 0) {

		//INFO("packet.size = %d 0x%h", packet.size, packet.size);
		//av_hex_dump_log(NULL, AV_LOG_INFO, packet.data, packet.size);

		if (src->key_frame)
			packet.flags |= AV_PKT_FLAG_KEY;

		packet.stream_index = out->sv->index;
		packet.pos = -1;

		if (packet.pts != AV_NOPTS_VALUE)
			packet.pts = av_rescale_q(packet.pts, out->ctx_cv->time_base, out->sv->time_base);

		if (packet.dts != AV_NOPTS_VALUE)
			packet.dts = av_rescale_q(packet.dts, out->ctx_cv->time_base, out->sv->time_base);

		if ((ret = av_interleaved_write_frame(out->ctx_f, &packet)) < 0) {
			ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
		}
		result = ret;
	}
	if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
	}
	av_free(buf);
	av_free(dst);

	return result;
}

int
muxer_encode_subtitle(muxer_t *out, char *text)
{
	int buf_max_size = 1024, buf_size = 0, ret = 0;

	const AVFilter *dt  = avfilter_get_by_name("drawtext");
	av_assert0(dt);
	AVFilterContext *ctx_fi;
	AVFilterGraph *fg;

	fg = avfilter_graph_alloc();
	av_assert0(fg);

	char *args = "text='StackOverflow': fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf: fontcolor=white: fontsize=24: box=1: boxcolor=black@0.5: boxborderw=5: x=(w-text_w)/2: y=(h-text_h)/2";


	if ((ret = avfilter_graph_create_filter(&ctx_fi, dt, "drawtext", args, NULL, fg)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}


	AVPacket packet;
	uint8_t* buf;
	AVSubtitle sub;
	sub.num_rects = 0;
	sub.pts = out->fc;
	sub.start_display_time = 0;
	//sub.end_display_time = 10;
	ass_add_rect(&sub, text);

	if ((buf = av_mallocz(buf_max_size)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}
	//buf[3] = 1;
	if ( (buf_size = avcodec_encode_subtitle(out->ctx_cs, buf, buf_max_size, &sub)) < 0) {
		ERR_EXIT("SUBTITLE:'%s' failed: %s", "avcodec_encode_subtitle", av_err2str(buf_size));
	}
	INFO("SUB: '%s' %d", buf, buf_size);
	av_hex_dump_log(NULL, AV_LOG_INFO, buf, buf_size);
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	packet.stream_index = out->ss->index;
	packet.pos = -1;
	packet.pts = out->fc;
	packet.dts = out->fc;
	packet.duration = 1000;



	if (packet.pts != AV_NOPTS_VALUE)
		packet.pts = av_rescale_q(packet.pts, out->ctx_cs->time_base, out->ss->time_base);

	if (packet.dts != AV_NOPTS_VALUE)
		packet.dts = av_rescale_q(packet.dts, out->ctx_cs->time_base, out->ss->time_base);

	// av_packet_rescale_ts(&packet, ifmt_ctx->streams[stream_index]->time_base, ofmt_ctx->streams[stream_index]->time_base);
	
	log_packet(out->ctx_f, &packet);

	if ((ret = av_interleaved_write_frame(out->ctx_f, &packet)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}

	av_free(buf);
	avsubtitle_free(&sub);
	return 0;
}