#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>


#include "log.h"
#include "err.h"
#include "vlt.h"
#include "utils.h"
#include "demuxer.h"
#include "muxer.h"
#include "ass.h"



static void vlt_subtitle (muxer_t *mux, int frame_num);

int
vlt_start (param_t *param) {

	if (param->debug)
		av_log_set_level(AV_LOG_DEBUG);

	int	ret =  0, frame_count = 1;
	int64_t start_time = 0;
	demuxer_t *inp = NULL;
	muxer_t *out = NULL;
	AVPacket packet;
	AVFrame *frame = NULL;


	inp = demuxer_new(param->file);
	out = muxer_new(param->stream, inp->width, inp->height);

	//return 0;

	if ((frame = av_frame_alloc()) == NULL) {
		ERR_EXIT("'%s' failed", "av_frame_alloc");
	}

	start_time = av_gettime();
	while (av_read_frame(inp->ctx_f, &packet) >= 0) {


		if (packet.stream_index == inp->siv) {

			AVRational time_base = inp->ctx_f->streams[inp->siv]->time_base;
			AVRational time_base_q = {1, AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time) {
				//INFO("Delaying %s", av_ts2str(pts_time - now_time));
				av_usleep(pts_time - now_time);
			}

			INFO("VIDEO: next %d (%d) (%d)", frame_count, out->ctx_cv->frame_number, out->ctx_cs->frame_number);

			if ((ret = avcodec_send_packet(inp->ctx_cv, &packet)) < 0) {
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
			}
			while ((ret = avcodec_receive_frame(inp->ctx_cv, frame)) >= 0) {
				//INFO("VIDEO: %s", "Decode SUCCESS");
				//log_frame(frame);
				AVFrame* tframe = NULL;
				int size = 0;
				uint8_t * buf = NULL;

				if ((tframe = av_frame_alloc()) == NULL) {
					ERR_EXIT("VIDEO:'%s' failed", "av_frame_alloc");
				}

				if ((size = av_image_get_buffer_size(out->pix_fmt, out->width, out->height, 1)) < 0) {
					ERR_EXIT("IMAGE:'%s' failed", "av_image_get_buffer_size");
				}

				if ((buf = (uint8_t*)av_mallocz(size)) == NULL) {
					ERR_EXIT("IMAGE:'%s' failed", "av_frame_alloc");
				}

				if ((ret = av_image_fill_arrays(tframe->data, tframe->linesize, buf, out->pix_fmt, out->width, out->height, 1)) < 0) {
					ERR_EXIT("IMAGE:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
				}

				av_image_copy(tframe->data, tframe->linesize, (const uint8_t**)frame->data, frame->linesize, out->pix_fmt, out->width, out->height);

				tframe->pts    = frame_count;
				tframe->format = out->pix_fmt;
				tframe->width  = out->width;
				tframe->height = out->height;
				if ((ret = avcodec_send_frame(out->ctx_cv, tframe)) < 0) {
					ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
				}

				AVPacket tpacket;
				av_init_packet(&tpacket);
				tpacket.data = NULL;
				tpacket.size = 0;
				tpacket.side_data_elems = 0;

				while ((ret = avcodec_receive_packet(out->ctx_cv, &tpacket)) >= 0) {
					//INFO("VIDEO: %s", "Encode SUCCESS");
					//log_packet(out->ctx_f, &tpacket);
					if (out->ctx_cv->coded_frame->key_frame)
						tpacket.flags |= AV_PKT_FLAG_KEY;

					tpacket.stream_index = out->sv->index;

					if (tpacket.pts != AV_NOPTS_VALUE)
						tpacket.pts = av_rescale_q(tpacket.pts, out->ctx_cv->time_base, out->sv->time_base);

					if (tpacket.dts != AV_NOPTS_VALUE)
						tpacket.dts = av_rescale_q(tpacket.dts, out->ctx_cv->time_base, out->sv->time_base);

					if ((ret = av_interleaved_write_frame(out->ctx_f, &tpacket)) < 0) {
						ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
					}

					vlt_subtitle(out, frame_count);

				}
				frame_count++;
				if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
					ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
				}
				av_free(buf);
				av_free(tframe);
			}
			if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
			}
		}
	}
	INFO("VIDEO:'%s' reading end: %s", "avcodec_read_frame", av_err2str(ret));
	av_write_trailer(out->ctx_f);

	//avio_seek(inp->ctx_f->pb, 0, SEEK_SET);
	//	avformat_seek_file(inp->ctx_f, inp->siv, 0, 0, inp->sv->duration, 0);
	//goto loop;
	av_free(frame);
	demuxer_free(inp);
	muxer_free(out);
	return ret;
}

static void
vlt_subtitle (muxer_t *mux, int frame_num) {
	int buf_max_size = 1024 * 1024, buf_size = 0, ret = 0;
	AVPacket packet;
	uint8_t* buf;
	AVSubtitle sub;
	ass_add_rect(&sub, "I am {\b1}StackOverflow[\b0} amused.");

	//	uint64_t startTime = sub.pts / 1000; // FIXME: need better solution...
	//	uint64_t endTime = startTime + sub.end_display_time;
	//	int start_tc[4] = {0}, end_tc[4] = {0};
	//	char text[1024] = {0};
	//
	//	if (make_tc(startTime, start_tc) || make_tc(endTime, end_tc)) {
	//		ERR_EXIT("'%s' failed", "make_tc");
	//	}
	//	snprintf(text, 1023,
	//			"ReadOrder=0,Layer=0,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,Default,NTP,0000,0000,0000,!Effect,%s{\\r}",
	//			start_tc[3], start_tc[2], start_tc[1], start_tc[0],
	//			end_tc[3],   end_tc[2],   end_tc[1],   end_tc[0],
	//			"I am {\b1}StackOverflow[\b0} amused.");


	sub.pts = frame_num;
	sub.start_display_time = 0;
	//sub.end_display_time = 10;

	if ((buf = av_mallocz(buf_max_size)) == NULL) {
		ERR_EXIT("'%s' failed", "av_mallocz");
	}

	if ( (buf_size = avcodec_encode_subtitle(mux->ctx_cs, buf, buf_max_size, &sub)) < 0) {
		ERR_EXIT("SUBTITLE:'%s' failed: %s", "avcodec_encode_subtitle", av_err2str(buf_size));
	}

	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	packet.stream_index = mux->ss->stream_identifier;

	if (packet.pts != AV_NOPTS_VALUE)
		packet.pts = av_rescale_q(packet.pts, mux->ctx_cs->time_base, mux->ss->time_base);

	if (packet.dts != AV_NOPTS_VALUE)
		packet.dts = av_rescale_q(packet.dts, mux->ctx_cs->time_base, mux->ss->time_base);

	//	packet.pts  = av_rescale_q(sub.pts, AV_TIME_BASE_Q, mux->ctx_cs->time_base);
	//	packet.duration = av_rescale_q(sub.end_display_time, (AVRational){1, 1000}, mux->ctx_cs->time_base);
	//	packet.dts = packet.pts;


	if ((ret = av_interleaved_write_frame(mux->ctx_f, &packet)) < 0) {
		ERR_EXIT("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
	}

	av_free(buf);
	avsubtitle_free(&sub);
}
