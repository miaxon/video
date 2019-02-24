#include "libavutil/avstring.h"
#include <libavutil/avassert.h>
#include "ass.h"
#include "log.h"

//static char* ass_get_dialog (int readorder, int layer, const char *style, const char *speaker, const char *text);
//static int make_tc (uint64_t ms, int *tc);

int
ass_subtitle_header (
	AVCodecContext *avctx,
	int playresx,
	int playresy,
	const char *font,
	int font_size,
	int color,
	int back_color,
	int bold,
	int italic,
	int underline,
	int border_style,
	int alignment)
{

	avctx->subtitle_header = (uint8_t*) av_asprintf(
		ASS_HEADER_STRING,
		!(avctx->flags & AV_CODEC_FLAG_BITEXACT) ? AV_STRINGIFY(LIBAVCODEC_VERSION) : "",
		playresx, playresy,
		font, font_size, color, color, back_color, back_color,
		-bold, -italic, -underline, border_style, alignment);

	if (!avctx->subtitle_header)
		return AVERROR(ENOMEM);
	avctx->subtitle_header_size = strlen((const char*) avctx->subtitle_header) + 1;
	INFO("SUBTITLE HEADER: %s", avctx->subtitle_header);
	return 0;
}

int
ass_subtitle_header_default (AVCodecContext *avctx)
{
	return ass_subtitle_header(avctx, 
		ASS_DEFAULT_PLAYRESX,
		ASS_DEFAULT_PLAYRESY,
		ASS_DEFAULT_FONT,
		ASS_DEFAULT_FONT_SIZE,
		ASS_DEFAULT_COLOR,
		ASS_DEFAULT_BACK_COLOR,
		ASS_DEFAULT_BOLD,
		ASS_DEFAULT_ITALIC,
		ASS_DEFAULT_UNDERLINE,
		ASS_DEFAULT_BORDERSTYLE,
		ASS_DEFAULT_ALIGNMENT);
}

//static char*
//ass_get_dialog (int marked, const char *style, const char *speaker, const char *text)
//{
//	return av_asprintf("0,0:00:00.00,1:00:00.00,Default,,0,0,0,,%s",
//		marked, style ? style : "Default",
//		speaker ? speaker : "", text);
//}

//static int
//make_tc (uint64_t ms, int *tc)
//{
//	static const int tc_divs[3] = { 1000, 60, 60 };
//	int i;
//	for (i = 0; i < 3; i++) {
//		tc[i] = ms % tc_divs[i];
//		ms /= tc_divs[i];
//	}
//	tc[3] = ms;
//	return ms > 99;
//}

int
ass_add_rect (AVSubtitle *sub, const char *text)
{
	
	char *ass_str;	
	sub->num_rects = 1;
	
	sub->rects = (AVSubtitleRect**) av_mallocz(sizeof(*sub->rects));
	av_assert0(sub->rects);
	
	sub->rects[0] = (AVSubtitleRect*) av_mallocz(sizeof(*sub->rects[0]));
	av_assert0(sub->rects[0]);
	
	sub->rects[0]->type = SUBTITLE_ASS;
	ass_str = av_asprintf(ASS_DIALOGUE_STRING, text);
	sub->rects[0]->ass = ass_str;
	
	return sub->num_rects;
}