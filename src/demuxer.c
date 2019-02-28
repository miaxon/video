#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>

#include "demuxer.h"
#include "err.h"
#include "log.h"

static AVFormatContext   *ctx_f;   // format context
static AVCodecContext    *ctx_cv;  // codec context video
static AVCodecContext    *ctx_ca;  // codec context video
static AVCodec           *cv;      // codec video
//static AVCodec           *ca;      // codec audio
static AVStream          *sv;      // stream video
static AVStream          *sa;

static AVFrame *vframe; // decoded video frame
static AVFrame *aframe; // decoded audio frame

static demuxer_t *mux;
static AVPacket  *packet;     // encoded data
static int64_t    start_time;

static int siv = -1;          // stream index video
static int sia = -1;          // stream index audio

static void demuxer_get_video_stream();
static void demuxer_get_audio_stream();
static void demuxer_delay();

demuxer_t *
demuxer_new (const char* name, int audio) {
	int	ret = 0;
	
	if ((mux = (demuxer_t*) av_mallocz(sizeof *mux)) == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "av_mallocz");
	}
	
	if ((mux->pa = (AVCodecParameters*) av_mallocz(sizeof *mux->pa)) == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "av_mallocz");
	}
	
	if ((packet = av_packet_alloc()) == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "av_packet_alloc");
	}
	
	if ((ret = avformat_open_input(&ctx_f, name, NULL, NULL)) != 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avformat_open_input", av_err2str(ret));
	}

	if (ctx_f == NULL) {
		ERR_EXIT("DEMUXER:%s", "Could not create input context");
	}

	if ((ret = avformat_find_stream_info(ctx_f, NULL)) < 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avformat_find_stream_info", av_err2str(ret));
	}
	demuxer_get_video_stream();
	mux->audio = audio;
	if (audio)
		demuxer_get_audio_stream();
	
	INFO("DEMUXER: audio stream %s", mux->audio ? "enabled": "disabled");
		
	start_time = av_gettime();
	av_dump_format(ctx_f, 0, name, 0);
	return mux;
}

static void
demuxer_get_audio_stream() {
	int ret = 0;
	if ((sia = av_find_best_stream(ctx_f, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0) {
		mux->audio = 0;
		ERROR("DEMUXER:'%s' failed: %s", "av_find_best_stream", av_err2str(sia));
		return;
	}
	sa = ctx_f->streams[sia];
	if ((ret = avcodec_parameters_copy(mux->pa, sa->codecpar)) < 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avcodec_parameters_copy", av_err2str(ret));
	}
	mux->atb = sa->time_base;
}

static void
demuxer_get_video_stream() {
	int ret = 0;

	if ((siv = av_find_best_stream(ctx_f, AVMEDIA_TYPE_VIDEO, -1, -1, &cv, 0)) == -1) {
		ERR_EXIT("DEMUXER:%s", "Didn't find a video input stream");
	}

	if (cv == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "Didn't find a video input decoder");
	}

	sv = ctx_f->streams[siv];

	if ((ctx_cv = avcodec_alloc_context3(cv)) == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "avcodec_alloc_context3");
	}

	if (ctx_cv == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "Didn't have video code context");
	}

	if ((ret = avcodec_parameters_to_context(ctx_cv, sv->codecpar)) < 0) {
		ERR_EXIT("'%s' failed: %s", "avcodec_parameters_to_context", av_err2str(ret));
	}

	if ((ret = avcodec_open2(ctx_cv, cv, NULL)) < 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avcodec_open2", av_err2str(ret));
	}

	if ((vframe = av_frame_alloc()) == NULL) {
		ERR_EXIT("DEMUXER:'%s' failed", "av_frame_alloc");
	}

	mux->width   = ctx_cv->width;
	mux->height  = ctx_cv->height;
	mux->pix_fmt = ctx_cv->pix_fmt;
	mux->vtb     = sv->time_base;
}

void
demuxer_free (void) {
	avcodec_close(ctx_cv);
	avcodec_close(ctx_ca);
	av_frame_free(&vframe);
	av_frame_free(&aframe);
	av_packet_unref(packet);
	avformat_close_input(&ctx_f);
}
/// Delay stream 

void
demuxer_delay(void) {
	// Delay
	AVRational time_base = ctx_f->streams[siv]->time_base;
	AVRational time_base_q = {1, AV_TIME_BASE};
	int64_t pts_time = av_rescale_q(packet->dts, time_base, time_base_q);
	int64_t now_time = av_gettime() - start_time;
	if (pts_time > now_time) {
		//INFO("Delaying %s", av_ts2str(pts_time - now_time));
		av_usleep(pts_time - now_time);
	}
}

packet_t
demuxer_read(void) {
	int ret = 0;

	ret = av_read_frame(ctx_f, packet);

	if (ret < 0) {
		INFO("DEMUXER:'%s': %s", "avcodec_read_frame", av_err2str(ret));
		return ret;
	}

	if (packet->stream_index == siv) {
		demuxer_delay();
		return PACKET_VIDEO;
	}

	if (packet->stream_index == sia) {
		return PACKET_AUDIO;
	}

	return PACKET_OTHER;

}

void
demuxer_unpack_video(void) {
	int ret = 0;
	if ((ret = avcodec_send_packet(ctx_cv, packet)) < 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
	}
}

void
demuxer_unpack_audio(void) {
	int ret = 0;
	if ((ret = avcodec_send_packet(ctx_ca, packet)) < 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avcodec_send_packet", av_err2str(ret));
	}
}

int
demuxer_decode_audio(void) {
	return avcodec_receive_frame(ctx_ca, aframe);
}

int
demuxer_decode_video(void) {
	return avcodec_receive_frame(ctx_cv, vframe);
}

AVFrame*
demuxer_get_frame_audio(void) {
	return aframe;
}

AVPacket*
demuxer_get_packet(void) {
	return packet;
}

AVFrame*
demuxer_get_frame_video(void) {
	return vframe;
}

void
demuxer_rewind(void) {
	int64_t ret = 0;	
	if ((ret = avio_seek(ctx_f->pb, 0, SEEK_SET)) < 0) {
		ERR_EXIT("DEMUXER:'%s' failed: %s", "avio_seek", av_err2str(ret));
	}
	avio_flush(ctx_f->pb);
}