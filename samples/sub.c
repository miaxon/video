// https://stackoverflow.com/questions/14485681/ffmpeg-not-honoring-bitrate-for-different-containers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>

#define STREAM_FRAME_RATE 25 /* 30 frames/s
#*/
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */

#define GV_ASS_DEFAULT_PLAYRESX 100 
#define GV_ASS_DEFAULT_PLAYRESY 100
#define GV_ASS_DEFAULT_COLOR 0xffffff
#define GV_ASS_DEFAULT_BACK_COLOR 0
#define GV_ASS_DEFAULT_BOLD 0
#define GV_ASS_DEFAULT_ITALIC 0
#define GV_ASS_DEFAULT_UNDERLINE 0
#define GV_ASS_DEFAULT_ALIGNMENT 2


// ./libavcodec/movtextenc.c

static char *header_format = 
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
"Format: Text\r\n";

/**************************************************************/ /* video output */
static AVFrame *frame;
static AVPicture src_picture, dst_picture;
static void open_video (AVFormatContext *oc, AVCodec *codec, AVStream *st) {
	int ret;
	AVCodecContext *c = st->codec;
	/* open the codec */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
		exit(1);
	}
	/* allocate and init a re-usable frame */
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	/* Allocate the encoded raw picture. */
	ret = avpicture_alloc(&dst_picture, c->pix_fmt, c->width, c->height);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate picture: %s\n", av_err2str(ret));
		exit(1);
	}
	/* If the output format is not YUV420P, then a temporary YUV420P
	 * picture is needed too. It is then converted to the required
	 * output format. */
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ret = avpicture_alloc(&src_picture, AV_PIX_FMT_YUV420P, c->width, c->height);
		if (ret < 0) {
			fprintf(stderr, "Could not allocate temporary picture: %s\n", av_err2str(ret));
			exit(1);
		}
	}
	/* copy data and linesize picture pointers to frame */
	//*((AVPicture *) frame) = dst_picture;
}
static void close_video (AVFormatContext *oc, AVStream *st) {
	avcodec_close(st->codec);
	av_free(src_picture.data[0]);
	av_free(dst_picture.data[0]);
	av_free(frame);
}
/**************************************************************/ /* Add an output stream. */ 
static AVStream *add_stream (AVFormatContext *oc, AVCodec **codec, enum AVCodecID 
codec_id) {
	AVCodecContext *c;
	AVStream *st;
	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
		exit(1);
	}
	st = avformat_new_stream(oc, *codec);
	if (!st) {
		fprintf(stderr, "Could not allocate stream\n");
		exit(1);
	}
	//st->id = oc->nb_streams - 1;
	c = st->codec;
	switch ((*codec)->type) {
		
		case AVMEDIA_TYPE_SUBTITLE:
		
			avcodec_get_context_defaults3(c, *codec);
			
			c->codec_id = codec_id;
			//c->width = 512;
			//c->height = 384;
			//c->bit_rate = 150 * 1000;
			c->time_base.den = STREAM_FRAME_RATE;
			c->time_base.num = 1;
			avcodec_open2(c, codec, NULL);
			printf("======================== dvb: %p\n", c->priv_data);
			break;		
		
		case AVMEDIA_TYPE_VIDEO:
			avcodec_get_context_defaults3(c, *codec);
			c->codec_id = codec_id;
			c->bit_rate = 150 * 1000;
			/* Resolution must be a multiple of two. */
			c->width = 512;
			c->height = 384;
			/* timebase: This is the fundamental unit of time (in seconds) in terms
			 * of which frame timestamps are represented. For fixed-fps content,
			 * timebase should be 1/framerate and timestamp increments should be
			 * identical to 1. */
			c->time_base.den = STREAM_FRAME_RATE;
			c->time_base.num = 1;
			c->gop_size = 12; /* emit one intra frame every twelve frames at most */
			c->pix_fmt = STREAM_PIX_FMT;
			if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
				/* just for testing, we also add B frames */
				c->max_b_frames = 2;
			}
			if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
				/* Needed to avoid using macroblocks in which some coeffs overflow.
				 * This does not happen with normal video, it just happens here as
				 * the motion of the chroma plane does not match the luma plane. */
				c->mb_decision = 2;
			}
			break;
		default:
			break;
	}
	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	return st;
}

void
log_packet (const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
	printf("Packet (size:%d pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n)",
			pkt->size,
			av_ts2str(pkt->pts),
			av_ts2timestr(pkt->pts, time_base),
			av_ts2str(pkt->dts),
			av_ts2timestr(pkt->dts, time_base),
			av_ts2str(pkt->duration),
			av_ts2timestr(pkt->duration, time_base),
			pkt->stream_index);
}

int main (int argc, char *argv[]) {
	// Decoder local variable declaration
	AVFormatContext *pFormatCtx = NULL;
	int i, videoStream;
	AVCodecContext *pCodecCtx = NULL;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVPacket packet;
	int frameFinished;
	// Encoder local variable declaration
	const char *filename;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *video_st;
	AVCodec *video_codec;
	AVStream *text_st;
	AVCodec *text_codec;
	
	AVStream *dvb_st;
	AVCodec *dvb_codec;
	
	int ret, frame_count;
	// Register all formats and codecs
	av_register_all();
	avcodec_register_all();
	// Open video file
	char *file = "dvb.ts"; //"sample.mp4"
	if (avformat_open_input(&pFormatCtx, file, NULL, NULL) != 0)
		return -1; // Couldn't open file
	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		return -1; // Couldn't find stream information
	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, file, 0);
	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream
	// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec (decoder)
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return -1; // Could not open codec
	// Allocate video frame
	pFrame = av_frame_alloc();
	// setup mux
	//filename = "output_file.flv";
	filename = "udp://127.0.0.1:1234?pkt_size=1316";
	fmt = av_guess_format("mpegts", NULL, NULL);
	if (fmt == NULL) {
		printf("Could not guess format.\n");
		return -1;
	}
	// allocate the output media context
	oc = avformat_alloc_context();
	if (oc == NULL) {
		printf("could not allocate context.\n");
		return -1;
	}
	// Set out format context to the format ffmpeg guessed
	oc->oformat = fmt;
	// Add the video stream using the h.264
	// codec and initialize the codec.
	video_st = NULL;
	video_st = add_stream(oc, &video_codec, AV_CODEC_ID_H264);
	
	dvb_st = NULL;
	dvb_st = add_stream(oc, &dvb_codec, AV_CODEC_ID_DVB_SUBTITLE);
	AVCodecParameters *p = pFormatCtx->streams[2]->codecpar;
	if(dvb_st->codec)
	avcodec_parameters_to_context(dvb_st->codec, p);
	//text_st = NULL;
	//text_st = add_stream(oc, &text_codec, AV_CODEC_ID_MOV_TEXT);
	// Now that all the parameters are set, we can open the
	// video codec and allocate the necessary encode buffers.
	if (video_st)
		open_video(oc, video_codec, video_st);
	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open '%s': %s\n", filename, av_err2str(ret));
			return 1;
		}
	}
	// dump output format
	av_dump_format(oc, 0, filename, 1);
	// Write the stream header, if any.
	ret = avformat_write_header(oc, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
		return 1;
	}
	// Read frames, decode, and re-encode
	frame_count = 1;
	int64_t start_time = av_gettime();
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == 2) {
			printf("sub packet\n");
			log_packet(pFormatCtx, &packet);
			av_hex_dump_log(NULL, AV_LOG_INFO, packet.data, 16);
			packet.stream_index = 1;
			ret = av_interleaved_write_frame(oc, &packet);
		}
		if (packet.stream_index == videoStream) {
			AVRational time_base = pFormatCtx->streams[videoStream]->time_base;
			AVRational time_base_q = {1, AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time) {
				av_usleep(pts_time - now_time);
			}
			// Decode video frame
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			if (ret < 0) {
				fprintf(stderr, "Error decoding video packet: %s\n", av_err2str(ret));
				exit(1);
			}
			// Did we get a video frame?
			if (frameFinished) {
				//printf("decode SUCCESS\n");
				// Initialize a new frame
				AVFrame* newFrame = av_frame_alloc();
				int size = avpicture_get_size(video_st->codec->pix_fmt, video_st->codec->width, video_st->codec->height);
				uint8_t* picture_buf = av_malloc(size);
				avpicture_fill((AVPicture *) newFrame, picture_buf, video_st->codec->pix_fmt, video_st->codec->width, video_st->codec->height);
				// Copy only the frame content without additional fields
				av_image_copy(newFrame->data, newFrame->linesize, pFrame->data, pFrame->linesize, video_st->codecpar->format, video_st->codecpar->width, video_st->codecpar->height);
				// encode the image
				
				
				
				
				//
				AVPacket pkt;
				int got_output;
				av_init_packet(&pkt);
				pkt.data = NULL; // packet data will be allocated by the encoder
				pkt.size = 0;
				// Set the frame's pts (this prevents the warning notice 'non-strictly-monotonic PTS')
				newFrame->pts = frame_count;
				newFrame->format = video_st->codecpar->format;
				newFrame->width = video_st->codecpar->width;
				newFrame->height = video_st->codecpar->height;
				ret = avcodec_encode_video2(video_st->codec, &pkt, newFrame, &got_output);
				if (ret < 0) {
					fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
					exit(1);
				}
				if (got_output) {
					if (video_st->codec->coded_frame->key_frame)
						pkt.flags |= AV_PKT_FLAG_KEY;
					pkt.stream_index = video_st->index;
					if (pkt.pts != AV_NOPTS_VALUE)
						pkt.pts = av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base);
					if (pkt.dts != AV_NOPTS_VALUE)
						pkt.dts = av_rescale_q(pkt.dts, video_st->codec->time_base, video_st->time_base);
					// Write the compressed frame to the media file.
					//log_packet(oc, &pkt);
					ret = av_interleaved_write_frame(oc, &pkt);
					
					//
				
					
					
				} else {
					printf("encode FAILED\n");
					ret = 0;
				}
				if (ret != 0) {
					fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
					exit(1);
				}
				//fprintf(stderr, "encoded frame #%d\n", frame_count);
				frame_count++;
				// Free the YUV picture frame we copied from the
				// decoder to eliminate the additional fields
				// and other packets/frames used
				av_free(picture_buf);
				av_free_packet(&pkt);
				av_free(newFrame);
			}
		}
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}
	/* Write the trailer, if any. The trailer must be written before you
	 * close the CodecContexts open when you wrote the header; otherwise
	 * av_write_trailer() may try to use memory that was freed on
	 * av_codec_close(). */
	av_write_trailer(oc);
	/* Close the video codec (encoder) */
	if (video_st)
		close_video(oc, video_st);
	// Free the output streams.
	for (i = 0; i < oc->nb_streams; i++) {
		av_freep(&oc->streams[i]->codec);
		av_freep(&oc->streams[i]);
	}
	if (!(fmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_close(oc->pb);
	/* free the output format context */
	av_free(oc);
	// Free the YUV frame populated by the decoder
	av_free(pFrame);
	// Close the video codec (decoder)
	avcodec_close(pCodecCtx);
	// Close the input video file
	avformat_close_input(&pFormatCtx);
	return 0;
}
