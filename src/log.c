#include <libavutil/timestamp.h>
#include <libavutil/pixdesc.h>
#include <libavutil/frame.h>

#include "log.h"

void
log_error (const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buf[1024] = {0};
	snprintf(buf, 1024, "[ERROR] %s\n", format);
	vprintf(buf, args);
	va_end(args);
}

void
log_info (const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buf[1024] = {0};
	snprintf(buf, 1024, "[INFO] %s\n", format);
	vprintf(buf, args);
	va_end(args);
}

void
log_packet (const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
	INFO("Packet (size:%d pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d)",
			pkt->size,
			av_ts2str(pkt->pts),
			av_ts2timestr(pkt->pts, time_base),
			av_ts2str(pkt->dts),
			av_ts2timestr(pkt->dts, time_base),
			av_ts2str(pkt->duration),
			av_ts2timestr(pkt->duration, time_base),
			pkt->stream_index);
}

void
log_frame (const AVFrame *frm) {
	INFO("Frame (type=%c, size=%d bytes dim=%dx%d) pts %s key_frame %d dts %s format %d (%s)",
			av_get_picture_type_char(frm->pict_type),
			frm->pkt_size,
			frm->width, frm->height,
			av_ts2str(frm->pts),
			frm->key_frame,
			av_ts2str(frm->pkt_dts),
			frm->format,
			av_get_pix_fmt_name(frm->format)
			);
}