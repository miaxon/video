
#ifndef ASS_SCR_H
#define ASS_SCR_H

#ifdef __cplusplus
extern "C" {
#endif
#define GV_ASS_DEFAULT_PLAYRESX 100
#define GV_ASS_DEFAULT_PLAYRESY 100
#define GV_ASS_DEFAULT_COLOR 0xffffff
#define GV_ASS_DEFAULT_BACK_COLOR 0
#define GV_ASS_DEFAULT_BOLD 0
#define GV_ASS_DEFAULT_ITALIC 0
#define GV_ASS_DEFAULT_UNDERLINE 0
#define GV_ASS_DEFAULT_ALIGNMENT 2

	const char *header_format =
		"[Script Info]\r\n"
		"; Script generated by FFmpeg/Lavc%s\r\n"
		"ScriptType: v4.00+\r\n"
		"PlayResX: %d\r\n"
		"PlayResY: %d\r\n"
		"\r\n"
		"[V4+ Styles]\r\n"

		/* ASSv4 header */
		"Format: Name, "
		"Fontname, Fontsize, "
		"PrimaryColour, SecondaryColour, OutlineColour, BackColour, "
		"Bold, Italic, Underline, StrikeOut, "
		"ScaleX, ScaleY, "
		"Spacing, Angle, "
		"BorderStyle, Outline, Shadow, "
		"Alignment, MarginL, MarginR, MarginV, "
		"Encoding\r\n"

		"Style: "
		"Default," /* Name */
		"%s,%d," /* Font{name,size} */
		"&H%x,&H%x,&H%x,&H%x," /* {Primary,Secondary,Outline,Back}Colour */
		"%d,%d,%d,0," /* Bold, Italic, Underline, StrikeOut */
		"100,100," /* Scale{X,Y} */
		"0,0," /* Spacing, Angle */
		"%d,1,0," /* BorderStyle, Outline, Shadow */
		"%d,10,10,10," /* Alignment, Margin[LRV] */
		"0\r\n" /* Encoding */

		"\r\n"
		"[Events]\r\n"
		"Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\r\n";


#ifdef __cplusplus
}
#endif

#endif /* ASS_SCR_H */

