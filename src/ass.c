#include "libavutil/avstring.h"
#include <libavutil/avassert.h>
#include "ass.h"
#include "log.h"
#include "err.h"


static ASS_Library  *lib;
static ASS_Renderer *rnd;
static ASS_Image    *img;

void
ass_init(int width, int height) {
	if ((lib = ass_library_init()) == NULL) {
		ERR_EXIT("ASS:'%s' failed", "ass_library_init");
	}

	if ((rnd = ass_renderer_init(lib)) == NULL) {
		ERR_EXIT("ASS:'%s' failed", "ass_render_init");
	}

	ass_set_use_margins(rnd, 0);

	ass_set_font_scale(rnd, 1.0 );

	ass_set_line_spacing(rnd, 0.0 );

	ass_set_hinting(rnd, ASS_HINTING_NONE );
	
	ass_set_frame_size(rnd, width, height);
	
	ass_set_fonts(rnd, NULL, "sans-serif", ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
}

ASS_Image*
ass_get_track(const char *sub) {

	char *buf  = av_asprintf(
			ASS_TRACK_STRING,
			AV_STRINGIFY(LIBAVCODEC_VERSION),
			ASS_DEFAULT_PLAYRESX,
			ASS_DEFAULT_PLAYRESY,
			ASS_DEFAULT_FONT,
			ASS_DEFAULT_FONT_SIZE,
			ASS_PRIMARY_COLOR,
			ASS_OUTLINE_COLOR,
			ASS_SECONDARY_COLOR,
			ASS_BACK_COLOR,
			ASS_DEFAULT_BOLD,
			ASS_DEFAULT_ITALIC,
			ASS_DEFAULT_UNDERLINE,
			ASS_DEFAULT_BORDERSTYLE,
			ASS_DEFAULT_ALIGNMENT,
			sub);
	//INFO("ASSTrack: %s", buf);
	int size = strlen((const char*) buf);

	ASS_Track *track = ass_read_memory(lib, buf, size, "utf-8");
	if (!track) {
		ERR_EXIT("ASS:'%s' failed", "ass_read_memory");
	}

	av_free(buf);
	
	img = ass_render_frame(rnd, track, 0, NULL);
	INFO("ASS img: dst_x %d, dst_y %d, stride %d, w %d, h %d", img->dst_x, img->dst_y, img->stride, img->w, img->h);
	
	return img;

}