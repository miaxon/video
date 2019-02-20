#ifndef LOG_H
#define LOG_H



#ifdef __cplusplus
extern "C" {
#endif
#include <syslog.h>
#include <stdarg.h>
#include <libavformat/avformat.h>

#define INFO(format, ...) log_info("%s:%d \t| " format, __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(format, ...) log_error("%s:%d \t| " format, __FILE__, __LINE__, __VA_ARGS__)
#define OK printf("%s:%d \t| ======== OK =========\n", __FILE__, __LINE__);

	void
	log_error(const char *format, ...);

	void
	log_info(const char *format, ...);
	
	void
	log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);

	void
	log_frame(const AVFrame *frm);
	
#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
