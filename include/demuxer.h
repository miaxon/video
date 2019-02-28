

#ifndef DEMUXER_H
#define DEMUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

    typedef enum {
        PACKET_UNKNOWN = -1,
        PACKET_VIDEO,
        PACKET_AUDIO,
        PACKET_OTHER
    } packet_t;

    typedef struct {
        int width;
        int height;
        int pix_fmt;
        int audio;
        AVCodecParameters *pa;
        AVRational atb;
        AVRational vtb;
    } demuxer_t;


    int
    demuxer_decode_video(void);
    int
    demuxer_decode_audio(void);
    void
    demuxer_free(void);
    AVFrame*
    demuxer_get_frame_audio(void);
    AVFrame*
    demuxer_get_frame_video(void);
    AVPacket*
    demuxer_get_packet(void);
    demuxer_t *
    demuxer_new(const char* name, int audio);
    packet_t
    demuxer_read(void);
    void
    demuxer_rewind(void);
    void
    demuxer_unpack_audio(void);
    void
    demuxer_unpack_video(void);


#ifdef __cplusplus
}
#endif

#endif /* DEMUXER_H */

