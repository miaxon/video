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
vlt_start(param_t *param) {
	av_log_set_level(AV_LOG_DEBUG);

	int	ret =  0, n = 0;
	int64_t start_time;
	demuxer_t *inp = NULL;
	muxer_t *out = NULL;

	AVPacket packet;
	AVFrame *frame = NULL;

	start_time = av_gettime();

	if ((frame = av_frame_alloc()) == NULL) {
		ERR_EXIT("'%s' failed", "av_frame_alloc");
	}

	inp = demuxer_new(param->file);
	out = muxer_new(param->stream, inp->width, inp->height);


	while (av_read_frame(inp->ctx_f, &packet) >= 0) {

		if (packet.stream_index == inp->siv) {

			AVRational time_base = inp->sv->time_base;
			AVRational time_base_q = {1, AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time) {
				//INFO("Delaying %s", av_ts2str(pts_time - now_time));
				av_usleep(pts_time - now_time);
			}

			INFO("VIDEO: next %d (%d)", n, out->ctx_cv->frame_number);

			if ((ret = avcodec_send_packet(inp->ctx_cv, &packet)) < 0) {
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
			}
			while ((ret = avcodec_receive_frame(inp->ctx_cv, frame)) >= 0) {
				INFO("%s", "Decode SUCCESS");
				log_frame(frame);

				av_image_copy(out->frame->data, out->frame->linesize, frame->data, frame->linesize, out->pix_fmt, out->width, out->height);
				
				

			}
			if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
			}
		}
	}

	return ret;
}


