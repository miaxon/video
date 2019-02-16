#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>

#include "log.h"
#include "err.h"
#include "vlt.h"
#include "utils.h"
#include "mux.h"
#include "mux_inp.h"
#include "mux_out.h"

static void vlt_delay (int64_t start_time, AVRational *time_base, int64_t dts);

static void
vlt_delay (int64_t start_time, AVRational *time_base, int64_t dts) {
	AVRational time_base_q = {1, AV_TIME_BASE};
	int64_t pts_time = av_rescale_q(dts, *time_base, time_base_q);
	int64_t now_time = av_gettime() - start_time;
	if (pts_time > now_time)
		av_usleep(pts_time - now_time);
}

int
vlt_start (param_t *param) {
	int	ret =  0;
	int64_t start_time = 0;
	muxer_t
			*mux_inp,
			*mux_out;
	AVPacket pkt;
	AVFrame  frm;

	if ( ( ret = utils_file_exists(param->file)) != 0) {
		ERROR("%s", "Invalid file path");
		return 1;
	}

	mux_inp = mux_inp_new(param->file);
	mux_out = mux_out_new(param->stream, "mpegts", AV_CODEC_ID_H264, mux_inp->param_audio->codec_id);

	/// Copy audio params
	if ((ret = avcodec_parameters_copy(mux_out->stream_audio->codecpar, mux_inp->param_audio)) < 0) {
		ERR_EXIT("'%s' failed", "avcodec_parameters_copy");
	}
	if ((ret = avcodec_parameters_to_context(mux_out->ctx_codec_audio, mux_inp->param_audio)) < 0) {
		ERR_EXIT("'%s' failed", "avcodec_parameters_to_context");
	}

	// Prepare encoder
	mux_out->stream_video->codec->pix_fmt   = mux_inp->stream_video->codec->pix_fmt;
	mux_out->stream_video->codec->flags     = mux_inp->stream_video->codec->flags;
	mux_out->stream_video->codec->width     = mux_inp->stream_video->codec->width;
	mux_out->stream_video->codec->height    = mux_inp->stream_video->codec->height;
	mux_out->stream_video->codec->time_base = mux_inp->stream_video->codec->time_base;
	mux_out->stream_video->codec->gop_size  = mux_inp->stream_video->codec->gop_size;
	mux_out->stream_video->codec->bit_rate  = mux_inp->stream_video->codec->bit_rate;
	mux_out->stream_video->codec->sample_rate  = mux_inp->stream_video->codec->sample_rate;
	/// Open output codec
	if ((ret = avcodec_open2(mux_out->ctx_codec_video, mux_out->codec_video, NULL)) < 0) {
		ERR_EXIT("'%s' failed", "avcodec_open2");
	}


	// Open output
	if (!(mux_out->ctx_format->oformat->flags & AVFMT_NOFILE)) {
		if ( (ret = avio_open(&mux_out->ctx_format->pb, param->file, AVIO_FLAG_WRITE)) < 0) {
			ERR_EXIT("%s", "'avio_open' failed");
		}
	}

	//Write file header
	if ( (ret = avformat_write_header(mux_out->ctx_format, NULL)) < 0) {
		ERR_EXIT("%s", "'avformat_write_header' failed");
	}

	start_time = av_gettime();
	while ((ret = av_read_frame(mux_inp->ctx_format, &pkt)) == 0) {

		if (pkt.stream_index != mux_inp->index_audio || pkt.stream_index != mux_inp->index_video) {
			av_packet_unref(&pkt);
			continue;
		}

		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(
				pkt.pts,
				mux_inp->stream_audio->time_base,
				mux_out->stream_audio->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

		pkt.dts = av_rescale_q_rnd(
				pkt.dts,
				mux_inp->stream_audio->time_base,
				mux_out->stream_audio->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

		pkt.duration = av_rescale_q(
				pkt.duration,
				mux_inp->stream_audio->time_base,
				mux_out->stream_audio->time_base);
		pkt.pos = -1;

		// Decode/Encode video
		if (pkt.stream_index == mux_inp->index_video) {
			vlt_delay(start_time, &mux_inp->stream_video->time_base, pkt.dts);
			// Decode
			if ((ret = avcodec_send_packet(mux_inp->ctx_codec_video, &pkt)) < 0) {
				ERR_EXIT("'%s' failed", "av_interleaved_write_frame");
			}
			while (ret >= 0) {
				ret = avcodec_receive_frame(mux_inp->ctx_codec_video, &frm);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				} else if (ret < 0) {
					ERR_EXIT("Error while receiving a frame from the decoder: %s", av_err2str(ret));
				}

			}
			// Encode
			if ((ret = avcodec_send_frame(mux_out->ctx_codec_video, &frm)) < 0) {
				ERR_EXIT("'%s' failed", "avcodec_send_frame");
			}
			while (ret >= 0) {
				ret = avcodec_receive_packet(mux_out->ctx_codec_video, &pkt);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				} else if (ret < 0) {
					ERR_EXIT("Error while receiving a packet from the encoder: %s", av_err2str(ret));
				}

			}

			if ((ret = av_interleaved_write_frame(mux_out->ctx_format, &pkt)) < 0) {
				ERR_EXIT("'%s' failed", "av_interleaved_write_frame");
			}
			av_frame_unref(&frm);
			av_packet_unref(&pkt);
		}

		// Copy audio packet without decode/encode
		if (pkt.stream_index == mux_inp->index_audio) {
			if ((ret = av_interleaved_write_frame(mux_out->ctx_format, &pkt)) < 0) {
				ERR_EXIT("'%s' failed", "av_interleaved_write_frame");
			}
			av_packet_unref(&pkt);
		}
	}
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

	if ( ( ret = utils_file_exists(param->file)) != 0) {
		ERROR("%s", "Invalid file path");
		return 1;
	}

	if ( (ret = avformat_open_input(&ifmt_ctx, param->file, NULL, NULL)) != 0) {
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
		out_stream->codecpar->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);

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

