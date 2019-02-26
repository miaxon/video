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

int
vlt_start (param_t *param) {

	if (param->debug)
		av_log_set_level(AV_LOG_DEBUG);

	int	ret =  0;
	int64_t start_time = 0;
	demuxer_t *inp = NULL;
	muxer_t *out = NULL;
	AVPacket packet;
	AVFrame *frame = NULL;


	inp = demuxer_new(param->file);
	out = muxer_new(param->stream, inp->pix_fmt, inp->width, inp->height);

	//return 0;

	if ((frame = av_frame_alloc()) == NULL) {
		ERR_EXIT("'%s' failed", "av_frame_alloc");
	}

	start_time = av_gettime();
	while (av_read_frame(inp->ctx_f, &packet) >= 0) {
		
		if (packet.stream_index == inp->siv) {
			// Delay
			AVRational time_base = inp->ctx_f->streams[inp->siv]->time_base;
			AVRational time_base_q = {1, AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time) {
				//INFO("Delaying %s", av_ts2str(pts_time - now_time));
				av_usleep(pts_time - now_time);
			}

			INFO("VIDEO: inp: %d out: %d dvb: %d", out->fc, out->sv->nb_frames);
			// Decode/Ecode
			if ((ret = avcodec_send_packet(inp->ctx_cv, &packet)) < 0) {
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
			}
			while ((ret = avcodec_receive_frame(inp->ctx_cv, frame)) >= 0) {

				muxer_encode_frame(frame, " text text texttexttext texttexttext text");

			}
			if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
				ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));

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
