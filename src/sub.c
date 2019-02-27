#include "libavutil/avstring.h"
#include <libavutil/avassert.h>
#include "sub.h"
#include "log.h"
#include "err.h"

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

static ASS_Library  *lib;
static ASS_Renderer *rnd;
static ASS_Image    *img;
static int width;
static int height;
static void sub_blend(AVFrame * frame, ASS_Image *img);
static ASS_Track* sub_get_track(const char *sub);

static ASS_Track*
sub_get_track(const char *sub) {

	char *buf  = av_asprintf(
		ASS_TRACK_STRING,
		AV_STRINGIFY(LIBAVCODEC_VERSION),
		width,
		height,
		ASS_DEFAULT_FONT,
		ASS_DEFAULT_FONT_SIZE,
		sub);
	int size = strlen((const char*) buf);
	//INFO("ASS: %s", buf);
	ASS_Track *track = ass_read_memory(lib, buf, size, "utf-8");
	if (!track) {
		ERR_EXIT("ASS:'%s' failed", "ass_read_memory");
	}
	av_free(buf);
	return track;
}

void
sub_init(int w, int h) {
	width = w;
	height = h;

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

int
sub_draw(AVFrame *frame) {
	// have new subtitle ??
	ASS_Track *track = sub_get_track("Hello, World! Я здесь.");
	img = ass_render_frame(rnd, track, 0, NULL);
	//INFO("ASS img: dst_x %d, dst_y %d, stride %d, w %d, h %d", img->dst_x, img->dst_y, img->stride, img->w, img->h);
	// else if previous image
	sub_blend(frame, img);
	ass_free_track(track);
	// else do nithing

	return 0;
}

static void
sub_blend(AVFrame *frame, ASS_Image *img) {
	int cnt = 0;
	while (img) {
		int x, y;
		unsigned char opacity = 255 - _a(img->color);
		unsigned char r = _r(img->color);
		unsigned char g = _g(img->color);
		unsigned char b = _b(img->color);

		unsigned char *src;
		unsigned char *dst;
		
		src = img->bitmap;
		dst = frame->data[0] + img->dst_y * frame->linesize[0] + img->dst_x * 3;
		for (y = 0; y < img->h; ++y) {
			for (x = 0; x < img->w; ++x) {
				unsigned k = ((unsigned) src[x]) * opacity / 255;
				// possible endianness problems
				dst[x * 3] = (k * b + (255 - k) * dst[x * 3]) / 255;
				dst[x * 3 + 1] = (k * g + (255 - k) * dst[x * 3 + 1]) / 255;
				dst[x * 3 + 2] = (k * r + (255 - k) * dst[x * 3 + 2]) / 255;
			}
			src += img->stride;
			dst += frame->linesize[0];
		}
		++cnt;
		img = img->next;
	}
}

void
sub_destroy(void) {
	ass_renderer_done(rnd);
	ass_library_done(lib);
}