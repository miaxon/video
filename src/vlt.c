#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>


#include "log.h"
#include "err.h"
#include "vlt.h"
#include "utils.h"
#include "demuxer.h"
#include "muxer.h"
#include "sub.h"

int
vlt_start (param_t *param) {

	if (param->debug)
		av_log_set_level(AV_LOG_DEBUG);

	int	ret =  PACKET_UNKNOWN;
	demuxer_t *inp = NULL;

	inp = demuxer_new(param->file);
	muxer_new(param->stream, inp);
	char *sub = param->title;
	//return 0;

	while ( (ret = demuxer_read()) >= 0) {
		//printf("read %d\n", ret);
		switch (ret) {
			case PACKET_AUDIO:
				continue; // do nothing
				break;
			case PACKET_VIDEO:
			{
				demuxer_unpack_video();
				while ((ret = demuxer_decode_video()) >= 0) { // transcode frame
					
					muxer_pack_video(demuxer_get_frame_video(), sub);
					
					while ((ret = muxer_encode_video()) >= 0) {
						muxer_write_video();
					}
					if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
						ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_packet", av_err2str(ret));
					
				}
				if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
					ERR_EXIT("VIDEO:'%s' failed: %s", "avcodec_receive_frame", av_err2str(ret));
			}
				break;
			default:
				continue; // do nothing
		}
	}

	INFO("VIDEO:'%s' reading end: %s", "avcodec_read_frame", av_err2str(ret));
	muxer_finish();

	//avio_seek(inp->ctx_f->pb, 0, SEEK_SET);
	//	avformat_seek_file(inp->ctx_f, inp->siv, 0, 0, inp->sv->duration, 0);
	//goto loop;
	demuxer_free();
	muxer_free();
	return ret;
}
