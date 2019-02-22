

#ifndef ASS_H
#define ASS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavutil/bprint.h>

#define ASS_DEFAULT_PLAYRESX 384
#define ASS_DEFAULT_PLAYRESY 288

#define ASS_DEFAULT_FONT        "Arial"
#define ASS_DEFAULT_FONT_SIZE   16
#define ASS_DEFAULT_COLOR       0xffffff
#define ASS_DEFAULT_BACK_COLOR  0
#define ASS_DEFAULT_BOLD        0
#define ASS_DEFAULT_ITALIC      0
#define ASS_DEFAULT_UNDERLINE   0
#define ASS_DEFAULT_ALIGNMENT   2
#define ASS_DEFAULT_BORDERSTYLE 1

	int
	ass_subtitle_header(
		AVCodecContext *avctx,
		const char *font, int font_size,
		int color, int back_color,
		int bold, int italic, int underline,
		int border_style, int alignment
		);

	int
	ass_subtitle_header_default(AVCodecContext *avctx);

	int
	ass_add_rect (AVSubtitle *sub, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* ASS_H */

