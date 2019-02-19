#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "log.h"
#include "err.h"
#include "vlt.h"
#include "utils.h"
#include "mux.h"
#include "mux_inp.h"
#include "mux_out.h"

/*static void vlt_delay (int64_t start_time, AVRational *time_base, int64_t dts);

static void
vlt_delay (int64_t start_time, AVRational *time_base, int64_t dts)
{
	AVRational time_base_q = {1, AV_TIME_BASE};
	int64_t pts_time = av_rescale_q(dts, *time_base, time_base_q);
	int64_t now_time = av_gettime() - start_time;
	if (pts_time > now_time)
		av_usleep(pts_time - now_time);
}*/

int
vlt_start (param_t *param) {
	av_log_set_level(AV_LOG_DEBUG);

	int	ret =  0, n = 0;
	//uint8_t * out[4] = { 0 };
	//int outLinesize[4] = { 0 };
	//int outSize;
	int64_t start_time = 0;
	muxer_t	*mux_inp, *mux_out;
	AVPacket *pkt = NULL;
	AVFrame  *frm = NULL;

	//struct SwsContext * sws = NULL;

	mux_inp = mux_inp_new(param->file);

	// allocate sws context
	//sws = sws_getContext(mux_inp->ctx_codec_video->width, mux_inp->ctx_codec_video->height, mux_inp->stream_video->codecpar->format, mux_inp->ctx_codec_video->width, mux_inp->ctx_codec_video->height, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
	//if (!sws) {
	//	ERR_EXIT("%s failed", "'sws_getContext'");
	//}


	mux_out = mux_out_new(param->stream, "mpegts", AV_CODEC_ID_H264, mux_inp);

	// Allocate frame and packetpkt->stream_index
	if ((pkt = av_packet_alloc()) == NULL) {
		ERR_EXIT("%s failed", "'av_packet_alloc'");
	}
	av_init_packet(pkt);
	if ((frm = av_frame_alloc()) == NULL) {
		ERR_EXIT("%s failed", "'av_frame_alloc'");
	}

	// alloc data for output frame
	//	ret = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, mux_inp->ctx_codec_video->width, mux_inp->ctx_codec_video->height, 1);
	//	if (ret < 0) ERR_EXIT("IMG:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	//	outSize = ret;
	//	out[0] = (uint8_t*)av_malloc((size_t)outSize);
	//	if (!out[0]) {
	//		ERR_EXIT("IMG:'%s' failed", "av_malloc");
	//	}
	//	ret = av_image_fill_arrays(out, outLinesize, out[0], AV_PIX_FMT_YUV420P, mux_inp->ctx_codec_video->width, mux_inp->ctx_codec_video->height, 1);
	//	if (ret < 0) ERR_EXIT("IMG:'%s' failed: %s", "av_image_fill_arrays", av_err2str(ret));
	//
	//	start_time = av_gettime();

	while ((ret = av_read_frame(mux_inp->ctx_format, pkt)) >= 0) {

		//log_packet(mux_inp->ctx_format, pkt);
		AVStream *in_stream, *out_stream;
		//int dts = pkt->dts;
		//FIX No PTS
		if (pkt->pts == AV_NOPTS_VALUE) {
			//Write PTS
			log_packet(mux_inp->ctx_format, pkt);
			in_stream = mux_inp->ctx_format->streams[mux_inp->index_video];
			AVRational time_base1 = in_stream->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
			//Parameters
			pkt->pts = (double) (n * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
			pkt->dts = pkt->pts;
			pkt->duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
		}

		in_stream  = mux_inp->ctx_format->streams[pkt->stream_index];
		out_stream = mux_out->ctx_format->streams[pkt->stream_index];
		pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
		pkt->pos = -1;

		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			//continue;
			if ((ret = av_interleaved_write_frame(mux_out->ctx_format, pkt)) < 0) {
				ERR_EXIT("AUDIO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
			}
			INFO("AUDIO: %s", "send audio packet");
			continue;
		}


		// Decode/Encode video
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			// Delay
			AVRational time_base = in_stream->time_base;
			AVRational time_base_q = {1, AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(pkt->dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time) {
				INFO("VIDEO: delaying %s", av_ts2str(pts_time - now_time));
				av_usleep(pts_time - now_time);
			}




			//av_frame_free(&frm);
			//if ((frm = av_frame_alloc()) == NULL) {
			//	ERR_EXIT("%s failed", "'av_frame_alloc'");
			//}

			//			int got_frame = 0;
			//			ret = avcodec_decode_video2(mux_inp->ctx_codec_video, frm, &got_frame, pkt);
			//			if (ret < 0) {
			//				ERR_EXIT("%s", "Error while decoding frame");
			//			}
			//
			//			log_frame(mux_inp->ctx_codec_video, frm);
			//			if(got_frame == 0) {
			//				INFO("'%s' [%d]: %s", "Frame decode failed", ret, av_err2str(ret));
			//				continue;
			//			}
			//sws_scale(sws, frm->data, frm->linesize, 0, mux_inp->ctx_codec_video->height, out, outLinesize);
			//av_frame_free(&frm);
			//frm = av_frame_alloc();
			//avpicture_fill((AVPicture *)frm, out, AV_PIX_FMT_YUV420P, mux_inp->ctx_codec_video->width, mux_inp->ctx_codec_video->height);
			//break;
			//}
			INFO("VIDEO: next %d", n);
			while ((ret = avcodec_send_packet(mux_inp->ctx_codec_video, pkt)) >= 0) {
				log_packet(mux_inp->ctx_format, pkt);
				while ((ret = avcodec_receive_frame(mux_inp->ctx_codec_video, frm)) >= 0) {
					log_frame(frm);
					frm->pts = ++n;
					while ((ret = avcodec_send_frame(mux_out->ctx_codec_video, frm)) >= 0) {

						AVPacket *_pkt;
						if ((_pkt = av_packet_alloc()) == NULL) {
							ERR_EXIT("%s failed", "'av_packet_alloc'");
						}
						av_init_packet(_pkt);
						_pkt->data = NULL; // packet data will be allocated by the encoder
						_pkt->size = 0;
						printf("Alloc new packet\n");
						while ((ret = avcodec_receive_packet(mux_out->ctx_codec_video, _pkt)) >= 0) {
							printf("Receive new packet\n");
							_pkt->dts = n + 41796;
							_pkt->pts =  ++n + 41796;
							_pkt->duration = pkt->duration;
							_pkt->pos = -1;
							if (frm->key_frame)
								_pkt->flags |= AV_PKT_FLAG_KEY;

							printf("Send new packet\n");
							log_packet(mux_out->ctx_format, _pkt);
							// Write the compressed frame to the media file.
							if ((ret = av_interleaved_write_frame(mux_out->ctx_format, _pkt)) < 0) {
								ERROR("VIDEO:'%s' failed: %s", "av_interleaved_write_frame", av_err2str(ret));
								av_packet_free(_pkt);
								break;
							}
							av_packet_free(_pkt);
							INFO("VIDEO: Sent %d", n);
						}
					}
				}
			}
			INFO("VIDEO: [%d] %s", ret, av_err2str(ret));
		}
	}

	//if (ret < 0) {
	//	INFO("'%s' [%d]: %s", "decode frame", ret, av_err2str(ret));
	//	continue;
	//}
	//av_packet_unref(pkt);
	//av_init_packet(pkt);
	//frm->pts = frame_num++;
	//			// Encode



	//			int got_packet = 0;
	//			ret = avcodec_encode_video2(mux_out->ctx_codec_video, pkt, frm, &got_packet);

	//			if (ret < 0 || pkt->dts <= 0) {
	//				ERROR("%s", "Error while encoding frame");
	//				//log_packet(mux_out->ctx_format, pkt);
	//				continue;
	//			}
	//			if(got_packet == 0) {
	//				INFO("'%s' [%d]: %s", "Frame encode failed", ret, av_err2str(ret));
	//				continue;
	//			}
	//log_packet(mux_out->ctx_format, pkt);

	av_packet_unref (pkt);

	av_frame_unref (frm);
	//frame_num++;


	av_write_trailer (mux_out->ctx_format);
	//av_frame_unref (frm);
	av_frame_free (&frm);
	//av_packet_unref (pkt);

	return ret;
}

int vlt_start1 (param_t *param) {
	int
	ret =  0,
			sid = -1,
			fid =  0;

	int64_t start_time = 0;

	AVFormatContext
			*ifmt_ctx = NULL,
			*ofmt_ctx = NULL;

	AVOutputFormat *ofmt = NULL;
	AVPacket pkt;
	AVCodec *icdc = NULL;
	AVDictionary *opts = NULL;

	if ( ( ret = utils_file_exists(param->file)) != 0) {
		ERROR("%s", "Invalid file path");
		return 1;
	}

	av_dict_set(&opts, "loop", "3", 0);
	if ( (ret = avformat_open_input(&ifmt_ctx, param->file, NULL, &opts)) != 0) {
		ERROR("%s [%d]", "'avformat_open_input' failed", ret);
		return 1;
	}

	if ( (ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		ERROR("%s [%d]", "'avformat_find_stream_info' failed", ret);
		goto cleanup;
	}

	sid = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &icdc, 0);
	if (sid == -1) {
		ERROR("%s", "Didn't find a video stream");
		ret = AVERROR_STREAM_NOT_FOUND;
		goto cleanup;
	}


	INFO("%s" , "dump input format");
	av_dump_format(ifmt_ctx, 0, param->file, 0);

	if ( (ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", param->stream)) < 0) {
		ERROR("%s [%d]", "'avformat_alloc_output_context2' failed", ret);
		goto cleanup;
	}

	if (!ofmt_ctx) {
		ERROR("%s", "Could not create output context");
		goto cleanup;
	}

	ofmt = ofmt_ctx->oformat;
	for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream
				*in_stream  = NULL,
				*out_stream = NULL;

		in_stream  = ifmt_ctx->streams[i];
		out_stream = avformat_new_stream(ofmt_ctx, icdc);
		if (!out_stream) {
			ERROR("%s", "Failed allocating output stream");
			goto cleanup;
		}
		//Copy the settings of AVCodecContext
		if ( (ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar)) < 0) {
			ERROR("%s", "Failed to copy context from input to output stream codec context");
			goto cleanup;
		}
		//out_stream->codecpar->codec_tag = 0;
		//if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		//	out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	}

	INFO("%s" , "dump output format");
	av_dump_format(ofmt_ctx, 0, param->stream, 1);
	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if ( (ret = avio_open(&ofmt_ctx->pb, param->stream, AVIO_FLAG_WRITE)) < 0) {
			ERROR("%s", "Could not open output URL '%s'", param->stream);
			goto cleanup;
		}
	}
	//Write file header
	if ( (ret = avformat_write_header(ofmt_ctx, NULL)) < 0) {
		ERROR("%s", "Error occurred when opening output URL");
		goto cleanup;
	}

	start_time = av_gettime();
	while (1) {
		AVStream
				*in_stream,
				*out_stream;
		//Get an AVPacket
		;
		if ( (ret = av_read_frame(ifmt_ctx, &pkt)) < 0)
			break;
		//FIX No PTS (Example: Raw H.264)
		//Simple Write PTS
		if (pkt.pts == AV_NOPTS_VALUE) {
			//Write PTS
			AVRational time_base1 = ifmt_ctx->streams[sid]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(ifmt_ctx->streams[sid]->r_frame_rate);
			//Parameters
			pkt.pts = (double) (fid * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
		}
		//Important:Delay
		if (pkt.stream_index == sid) {
			AVRational time_base = ifmt_ctx->streams[sid]->time_base;
			AVRational time_base_q = {1, AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time) {
				//INFO("Delaying %s", av_ts2str(pts_time - now_time));
				av_usleep(pts_time - now_time);
			}

		}

		in_stream  = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		/* copy packet */
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		//Print to Screen
		if (pkt.stream_index == sid) {

			/*AVSubtitle sub;
			sub.rects = (AVSubtitleRect**) av_mallocz(2 * sizeof(*sub.rects));
			sub.rects[0] = (AVSubtitleRect*) av_mallocz(sizeof(*sub.rects[0]));
			sub.num_rects = 1;
			sub.start_display_time = 0;
			sub.rects[0]->type = SUBTITLE_TEXT;
			sub.rects[0]->text = "StackOverflow";

			uint64_t bufferSize = 1024 * 1024 ;
			uint8_t *buffer = (uint8_t*) av_mallocz(bufferSize);
			if( (ret = avcodec_open2(out_stream->codec, out_stream->codec->codec, NULL )) < 0) {
				ERROR( "%s", "'avcodec_open2' failed");
				break;
			}
			if( (ret = avcodec_encode_subtitle(out_stream->codec, buffer, bufferSize, &sub )) < 0) {
				ERROR( "%s", "Error muxing packet");
				break;
			}
			//pkt.data = buffer;
			//pkt.size = ret;*/

			INFO("Send %8d video frames to output URL", fid);
			fid++;
		}
		//ret = av_write_frame(ofmt_ctx, &pkt);


		if ( (av_interleaved_write_frame(ofmt_ctx, &pkt)) < 0) {
			ERROR( "%s", "Error muxing packet");
			break;
		}

		av_packet_unref(&pkt);

	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);

cleanup:

	avformat_close_input(&ifmt_ctx);

	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);

	avformat_free_context(ofmt_ctx);
	return ret;
}

