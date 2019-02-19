
#include<libavutil/avutil.h>
#include<libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>
#include <libavutil/pixdesc.h>


int main(int argc, char **argv) {
    AVFormatContext* ctx_format = NULL;
    AVCodecContext* ctx_codec = NULL;
    AVCodec* codec = NULL;
    AVFrame* frame = av_frame_alloc();
    int stream_idx;
    const char* fin = argv[1];
    AVStream *vid_stream = NULL;
    AVPacket* pkt = av_packet_alloc();
int ret = 0;
    if ((ret = avformat_open_input(&ctx_format, fin, NULL, NULL)) != 0) {
        
        return ret;
    }
    if (avformat_find_stream_info(ctx_format, NULL) < 0) {
        
        return -1; // Couldn't find stream information
    }
    av_dump_format(ctx_format, 0, fin, 0);

    for (int i = 0; i < ctx_format->nb_streams; i++)
        if (ctx_format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            stream_idx = i;
            vid_stream = ctx_format->streams[i];
            break;
    }
    if (vid_stream == NULL) {
        return -1;
    }

    codec = avcodec_find_decoder(vid_stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    ctx_codec = avcodec_alloc_context3(codec);

    if(avcodec_parameters_to_context(ctx_codec, vid_stream->codecpar)<0)
        fprintf(stderr, "codec not found\n");
    if (avcodec_open2(ctx_codec, codec, NULL)<0) {
        fprintf(stderr, "codec not found\n");
        return -1;
    }

    //av_new_packet(pkt, pic_size);

    while(av_read_frame(ctx_format, pkt) >= 0){
    if (pkt->stream_index == stream_idx) {
        int ret = avcodec_send_packet(ctx_codec, pkt);
        if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        while (ret  >= 0) {
            ret = avcodec_receive_frame(ctx_codec, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
        }
        printf("Frame %d (type=%c, size=%d bytes dim=%dx%d) pts %s key_frame %d [DTS %d] format %d (%s) codec %s (%s)\n",
			ctx_codec->frame_number,
			av_get_picture_type_char(frame->pict_type),
			frame->pkt_size,
			frame->width, frame->height,
			av_ts2str(frame->pts),
			frame->key_frame,
			frame->coded_picture_number,
			frame->format,			
			av_get_pix_fmt_name(frame->format),
			ctx_codec->codec->name,
			av_get_pix_fmt_name(ctx_codec->pix_fmt)
			);
    }
    av_packet_unref(pkt);
    }

    avformat_close_input(&ctx_format);
    av_packet_unref(pkt);
    avcodec_free_context(&ctx_codec);
    avformat_free_context(ctx_format);
    return 0;
}
