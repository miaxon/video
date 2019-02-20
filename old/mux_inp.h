#ifndef VLT_MUXER_H
#define VLT_MUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mux.h"
	muxer_t *
	mux_inp_new(const char* name);

	void
	mux_inp_free(muxer_t* mux);

#ifdef __cplusplus
}
#endif

#endif /* VLT_MUXER_H */

