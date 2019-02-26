

#ifndef DEMUXER_H
#define DEMUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

	typedef enum {
		PACKET_UNKNOWN = -1,
		PACKET_VIDEO,
		PACKET_AUDIO,
		PACKET_OTHER
	} packet_t;

	typedef struct {
		AVCodecParameters *pv;
		AVCodecParameters *pa;
		int width;
		int height;
		int pix_fmt;
	} demuxer_t;

	
	int
	demuxer_decode_video(void);
	int
	demuxer_decode_audio(void);
	void
	demuxer_free(void);
	AVFrame*
	demuxer_get_frame_audio(void);
	AVFrame*
	demuxer_get_frame_video(void);
	demuxer_t *
	demuxer_new(const char* name);	
	packet_t
	demuxer_read(void);	
	void
	demuxer_unpack_audio(void);
	void
	demuxer_unpack_video(void);


#ifdef __cplusplus
}
#endif

#endif /* DEMUXER_H */

