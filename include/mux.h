
#ifndef MUX_H
#define MUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

	typedef struct {
		AVFormatContext   *ctx_format;
		AVCodecContext    *ctx_codec_video;
		AVCodecContext    *ctx_codec_audio;
		AVCodec           *codec_video;
		AVCodec           *codec_audio;
		AVStream          *stream_video;
		AVStream          *stream_audio;
		AVCodecParameters *param_video;
		AVCodecParameters *param_audio;
		int                index_video;
		int                index_audio;
	} muxer_t;


#ifdef __cplusplus
}
#endif

#endif /* MUX_H */

