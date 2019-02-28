
#ifndef MUXER_H
#define MUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h> 
#include "demuxer.h"

	typedef struct {
		int  fc;     // frame count
		int  width;
		int  height;
		int  pix_fmt_inp;
		int  pix_fmt_out;
	} muxer_t;
	
	void
	muxer_finish(void);
	void
	muxer_free(void);
	muxer_t *
	muxer_new(const char* name, demuxer_t *demux);        
	int
	muxer_pack_video(AVFrame *src, const char* sub);
	int
	muxer_encode_video(void);
	void
	muxer_write_audio(AVPacket *p);
	void
	muxer_write_video(void);
#ifdef __cplusplus
}
#endif

#endif /* MUXER_H */

