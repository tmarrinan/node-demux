#include "loadworker.h"

void LoadWorker::Execute() {
	baton->state = DS_LOAD;
	baton->filename = filename;
	baton->decode_first_frame = decodeFirstFrame;
	OpenVideoFile();
}

void LoadWorker::HandleOKCallback() {
	NanScope();
	
	if(baton->error != "") {
		baton->m_Error(baton->error);
	}
	else {
		baton->m_MetaData();
		baton->state = DS_IDLE;
		switch (baton->action) {
			case DA_LOAD:
				if(baton->decode_first_frame) {
					NanAsyncQueueWorker(new DemuxWorker(baton, false));
				}
				baton->action = DA_NONE;
				break;
			case DA_PLAY:
				baton->demux_start = uv_now(uv_default_loop());
				baton->video_start = baton->current_frame * baton->frame_time * 1000.0;
				//obj->baton->paused = false;
				baton->m_Start();
				NanAsyncQueueWorker(new DemuxWorker(baton, true));
				break;
			case DA_PAUSE:
				break;
			case DA_SEEK:
				break;
			default:
				break;
		}
	}
}

void LoadWorker::OpenVideoFile() {
	int ret = 0;
	
	// open input file, and allocate format context
	ret = avformat_open_input(&baton->fmt_ctx, baton->filename.c_str(), NULL, NULL);
	if (ret < 0) { baton->error = "could not open source file: " + baton->filename; return; }
	ret = avformat_find_stream_info(baton->fmt_ctx, NULL);
	if (ret < 0) { baton->error = "could not find stream information"; return; }
	ret = OpenCodecContext(&baton->video_stream_idx, baton->fmt_ctx);
	if (ret < 0) { return; }
	baton->video_stream = baton->fmt_ctx->streams[baton->video_stream_idx];
	baton->video_dec_ctx = baton->video_stream->codec;
	
	// get video metadata
	baton->width  = baton->video_dec_ctx->width;
	baton->height = baton->video_dec_ctx->height;
	baton->display_aspect_ratio = (double)(baton->width * baton->video_dec_ctx->sample_aspect_ratio.num) / (double)(baton->height * baton->video_dec_ctx->sample_aspect_ratio.den);
	baton->num_frames = baton->video_stream->nb_frames;
	baton->duration = (double)baton->fmt_ctx->duration / (double)AV_TIME_BASE;
	baton->frame_rate = (double)baton->video_stream->avg_frame_rate.num/(double)baton->video_stream->avg_frame_rate.den;
	baton->frame_time = 1.0 / baton->frame_rate;
	baton->video_time_base = (double)baton->video_stream->time_base.num / (double)baton->video_stream->time_base.den;
	if(baton->display_aspect_ratio <= 0) baton->display_aspect_ratio = (double)baton->width / (double)baton->height;
	if(baton->num_frames <= 0) baton->num_frames = (int64_t)floor((baton->duration * baton->frame_rate) + 0.5);
	
	if      (baton->video_dec_ctx->pix_fmt == PIX_FMT_YUV420P) baton->format = "yuv420p";
	else if (baton->video_dec_ctx->pix_fmt == PIX_FMT_RGB24)   baton->format = "rgb24";
	else if (baton->video_dec_ctx->pix_fmt == PIX_FMT_RGB32)   baton->format = "rgb32";
	else                                                       baton->format = "unknown";

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)	
	baton->frame = av_frame_alloc();
#else
	baton->frame = avcodec_alloc_frame();
#endif
	if (!baton->frame) { baton->error = "could not allocate frame"; return; }
    
    baton->pkt.data = NULL;
	baton->pkt.size = 0;
	
	//baton->paused = true;
	//baton->new_frame = false;
	//baton->cue_in_frame = -1;
	baton->current_frame= -1;
	av_init_packet(&baton->pkt);
}

int LoadWorker::OpenCodecContext(int *stream_idx, AVFormatContext *fctx) {
	int ret;
    AVStream *st;
    AVCodecContext *cctx = NULL;
    AVCodec *codec = NULL;
    ret = av_find_best_stream(fctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) { baton->error = "could not find video stream in input file"; return -1; }
    
	*stream_idx = ret;
	st = fctx->streams[*stream_idx];
	// find decoder for the stream
	cctx = st->codec;
	codec = avcodec_find_decoder(cctx->codec_id);
	if (!codec) { baton->error = "failed to find codec"; return -1; };
	ret = avcodec_open2(cctx, codec, NULL);
	if (ret < 0) { baton->error = "failed to open codec"; return -1; }
	
    return 0;
}
