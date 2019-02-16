#ifndef MUX_OUT_H
#define MUX_OUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include "mux.h"
	muxer_t *
	mux_out_new(const char* name, const char *format, int video_codec_id, int audio_codec_id);

	void
	mux_out_free(muxer_t* mux);

#ifdef __cplusplus
}
#endif

#endif /* MUX_OUT_H */

