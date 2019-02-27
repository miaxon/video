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
#include "demuxer.h"
#include "muxer.h"
#include "net.h"

static void vlt_loop();

static void
vlt_loop() {
	
	int	ret =  PACKET_UNKNOWN;
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

					muxer_pack_video(demuxer_get_frame_video(), net_get_title());

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
}

int
vlt_start (param_t *param) {

	int loop = param->loop;
	if (param->debug)
		av_log_set_level(AV_LOG_DEBUG);

	demuxer_t *inp = NULL;

	inp = demuxer_new(param->file);
	muxer_new(param->stream, inp);	
	net_init(param->title, param->port, param->url);

	if(loop > 0) {
		while(loop--) {
			vlt_loop();
			if(loop != 0)
				demuxer_rewind();
		}
	} else {
		vlt_loop();
		demuxer_rewind();
	}

	muxer_finish();
	demuxer_free();
	muxer_free();

	return 0;
}
