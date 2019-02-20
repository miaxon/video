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

/*static void vlt_delay (int64_t start_time, AVRational *time_base, int64_t dts);

static void
vlt_delay (int64_t start_time, AVRational *time_base, int64_t dts)
{
	AVRational time_base_q = {1, AV_TIME_BASE};
	int64_t pts_time = av_rescale_q(dts, *time_base, time_base_q);
	int64_t now_time = av_gettime() - start_time;
	if (pts_time > now_time)
		av_usleep(pts_time - now_time);
}*/

int
vlt_start (param_t *param) {
	//av_log_set_level(AV_LOG_DEBUG);

	int	ret =  0, frame_count = 1;
	int64_t start_time;
	demuxer_t *inp = NULL;
	muxer_t *out = NULL;

	AVPacket packet;
	AVFrame *frame = NULL;



	if ((frame = av_frame_alloc()) == NULL) {
		ERR_EXIT("'%s' failed", "av_frame_alloc");
	}

	inp = demuxer_new(param->file);
	out = muxer_new(param->stream, inp->width, inp->height);

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

			INFO("VIDEO: next %d (%d)", frame_count, out->ctx_cv->frame_number);

			if ((ret = avcodec_send_packet(inp->ctx_cv, &packet)) < 0) {
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
			}
			while ((ret = avcodec_receive_frame(inp->ctx_cv, frame)) >= 0) {
				INFO("VIDEO: %s", "Decode SUCCESS");
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

				tframe->pts = frame_count++;
				if ((ret = avcodec_send_frame(out->ctx_cv, tframe)) < 0) {
					ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
				}

				AVPacket tpacket;
				av_init_packet(&tpacket);
				tpacket.data = NULL;
				tpacket.size = 0;
				tpacket.side_data_elems = 0;

				while ((ret = avcodec_receive_packet(out->ctx_cv, &tpacket)) >= 0) {
					INFO("VIDEO: %s", "Encode SUCCESS");
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
				}
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
	av_write_trailer(out->ctx_f);
	return ret;
}


