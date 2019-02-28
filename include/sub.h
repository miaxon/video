#ifndef SUB_H
#define SUB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavutil/bprint.h>
#include <ass/ass.h>

	void
	sub_destroy(void);
	void 
	sub_init(int width, int height);
	int
	sub_draw(AVFrame *frame, const char* sub);

#ifdef __cplusplus
}
#endif

#endif /* SUB_H */

