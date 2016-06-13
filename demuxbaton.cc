#include "demuxbaton.h"

DemuxBaton::DemuxBaton() {
	timerReq.data = this;
	uv_timer_init(uv_default_loop(), &timerReq);
	
	fmt_ctx           = NULL;
	video_dec_ctx     = NULL;
	video_stream      = NULL;
	frame             = NULL;
	video_stream_idx  = -1;
	frame_buffer = new VideoFrame();
	error = "";
	
	def_err   = false;
	def_start = false;
	def_end   = false;
	def_frame = false;
	
	state = DS_IDLE;
	action = DA_NONE;
	
	PauseCallback = NULL;
	SeekCallback = NULL;
}

DemuxBaton::~DemuxBaton() {

}


void DemuxBaton::m_Error(std::string msg) {
	Nan::HandleScope();

	if (def_err) {
		Local<Value> argv[1] = { Nan::New<String>(msg.c_str()).ToLocalChecked() };
		OnError->Call(1, argv);
	}
}

void DemuxBaton::m_MetaData() {
	Nan::HandleScope();
	
	if (def_meta) {			
		Local<Object> meta = Nan::New<Object>();
		meta->Set(Nan::New<String>("width").ToLocalChecked(),                Nan::New<Number>(width));
		meta->Set(Nan::New<String>("height").ToLocalChecked(),               Nan::New<Number>(height));
		meta->Set(Nan::New<String>("display_aspect_ratio").ToLocalChecked(), Nan::New<Number>(display_aspect_ratio));
		meta->Set(Nan::New<String>("num_frames").ToLocalChecked(),           Nan::New<Number>(num_frames));
		meta->Set(Nan::New<String>("frame_rate").ToLocalChecked(),           Nan::New<Number>(frame_rate));
		meta->Set(Nan::New<String>("duration").ToLocalChecked(),             Nan::New<Number>(duration));
		meta->Set(Nan::New<String>("pixel_format").ToLocalChecked(),         Nan::New<String>(format.c_str()).ToLocalChecked());
		meta->Set(Nan::New<String>("convert_format").ToLocalChecked(),       Nan::New<String>(colorspace.c_str()).ToLocalChecked());
		Local<Value> argv[1] = {meta};
		OnMetaData->Call(1, argv);
	}
}

void DemuxBaton::m_Start() {
	Nan::HandleScope();
	
	if (def_start) {
		OnStart->Call(0, NULL);
	}
}

void DemuxBaton::m_End() {
	Nan::HandleScope();
	
	if (def_end) {
		OnEnd->Call(0, NULL);
	}
}

void DemuxBaton::m_Frame(VideoFrame *frm) {
	Nan::HandleScope();
	
	if (def_frame) {
		uint32_t size = frm->getBufferSize();
		const char *buf = reinterpret_cast<const char*>(frm->getBuffer());
		int64_t frameIdx = frm->getFrameIndex();
		
		if (colorspace == "rgb24" && format != "rgb24") {
			int output_bufferSize = avpicture_get_size(AV_PIX_FMT_RGB24, width, height);
			uint8_t *outData[1] = { output_buffer }; // RGB24 have one plane
			int outLinesize[1] = { 3 * width }; // RGB24 stride

			sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, height, outData, outLinesize);

			Local<Value> argv[2] = { Nan::New<Number>(frameIdx), Nan::CopyBuffer((const char *)output_buffer, output_bufferSize).ToLocalChecked() };
			OnFrame->Call(2, argv);
		}
		else if (colorspace == "yuv420p" && format != "yuv420p") {
			int output_bufferSize = avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
			uint8_t *outData[3] = { output_buffer, output_buffer+(width*height), output_buffer+(width*height)+((width/2)*(height/2))}; // YUV420P have three planes
			int outLinesize[3] = { width, width/2, width/2 }; // YUV420P stride
			sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, height, outData, outLinesize);

			Local<Value> argv[2] = { Nan::New<Number>(frameIdx), Nan::CopyBuffer((const char *)output_buffer, output_bufferSize).ToLocalChecked() };
			OnFrame->Call(2, argv);
		}
		else {
			Local<Value> argv[2] = { Nan::New<Number>(frameIdx), Nan::CopyBuffer(buf, size).ToLocalChecked() };	
			OnFrame->Call(2, argv);
		}
	}
}

void DemuxBaton::m_Pause() {
	Nan::HandleScope();
	
	PauseCallback->Call(0, NULL);
	delete PauseCallback;
	PauseCallback = NULL;
}

void DemuxBaton::m_Seek() {
	Nan::HandleScope();
	
	SeekCallback->Call(0, NULL);
	delete SeekCallback;
	SeekCallback = NULL;
}


void DemuxBaton::OpenVideoFile() {
	int ret = 0;
	
	// open input file, and allocate format context
	ret = avformat_open_input(&fmt_ctx, filename.c_str(), NULL, NULL);
	if (ret < 0) { error = "could not open source file: " + filename; return; }
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0) { error = "could not find stream information"; return; }
	ret = OpenCodecContext(&video_stream_idx, fmt_ctx);
	if (ret < 0) { return; }
	video_stream = fmt_ctx->streams[video_stream_idx];
	video_dec_ctx = video_stream->codec;
	
	// get video metadata
	width  = video_dec_ctx->width;
	height = video_dec_ctx->height;
	display_aspect_ratio = (double)(width * video_dec_ctx->sample_aspect_ratio.num) / (double)(height * video_dec_ctx->sample_aspect_ratio.den);
	num_frames = video_stream->nb_frames;
	duration = (double)fmt_ctx->duration / (double)AV_TIME_BASE;
	frame_rate = (double)video_stream->avg_frame_rate.num/(double)video_stream->avg_frame_rate.den;
	frame_time = 1.0 / frame_rate;
	video_time_base = (double)video_stream->time_base.num / (double)video_stream->time_base.den;
	if(display_aspect_ratio <= 0) display_aspect_ratio = (double)width / (double)height;
	if(num_frames <= 0) num_frames = (int64_t)floor((duration * frame_rate) + 0.5);
	
	src_pix_fmt = video_dec_ctx->pix_fmt;

	if      (video_dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P) format = "yuv420p";
	else if (video_dec_ctx->pix_fmt == AV_PIX_FMT_RGB24)   format = "rgb24";
	else if (video_dec_ctx->pix_fmt == AV_PIX_FMT_RGB32)   format = "rgb32";
	else                                                   format = "unknown";

	if (colorspace == "rgb24" && format != "rgb24") {
		output_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_RGB24, width, height));
		img_convert_ctx = sws_getContext(width, height, src_pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 
		if (img_convert_ctx == NULL) { error = "could not convert colorspace"; return; }
	}
	else if (colorspace == "yuv420p" && format != "yuv420p") {
		output_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, width, height));
		img_convert_ctx = sws_getContext(width, height, src_pix_fmt, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
		if (img_convert_ctx == NULL) { error = "could not convert colorspace"; return; }
	}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)	
	frame = av_frame_alloc();
#else
	frame = avcodec_alloc_frame();
#endif
	if (!frame) { error = "could not allocate frame"; return; }
    
    pkt.data = NULL;
	pkt.size = 0;
	
	current_frame= -1;
	av_init_packet(&pkt);
}

int DemuxBaton::OpenCodecContext(int *stream_idx, AVFormatContext *fctx) {
	int ret;
    AVStream *st;
    AVCodecContext *cctx = NULL;
    AVCodec *codec = NULL;
    ret = av_find_best_stream(fctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) { error = "could not find video stream in input file"; return -1; }
    
	*stream_idx = ret;
	st = fctx->streams[*stream_idx];
	// find decoder for the stream
	cctx = st->codec;
	codec = avcodec_find_decoder(cctx->codec_id);
	if (!codec) { error = "failed to find codec"; return -1; };
	ret = avcodec_open2(cctx, codec, NULL);
	if (ret < 0) { error = "failed to open codec"; return -1; }
	
    return 0;
}

void DemuxBaton::DecodeFrame() {
	int ret = 0, got_frame = 0;
	
	while (true) {
		// read new packet if empty
		if (pkt.size <= 0) {
            ret = av_read_frame(fmt_ctx, &pkt);
			if (ret < 0) break;
			orig_pkt = pkt;
		}
		do {
			ret = DecodePacket(&got_frame, 0);
			if (ret < 0) break;
			pkt.data += ret;
			pkt.size -= ret;
		} while (pkt.size > 0 && !got_frame);
		if (pkt.size <= 0) av_free_packet(&orig_pkt);
        if (got_frame) return;
	}
	
	// flush cached frames
	pkt.data = NULL;
	pkt.size = 0;
	DecodePacket(&got_frame, 1);
	if (!got_frame) action = DA_END;
}

int DemuxBaton::DecodePacket(int *got_frame, int cached) {
	int ret = 0;
    int decoded = pkt.size;
    if (pkt.stream_index == video_stream_idx) {
		// decode video frame
		ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
		if(ret < 0) { error = "could not decode video frame"; return -1; }
		if (*got_frame) {
			if (frame->pkt_dts >= 0) {
				current_time = (double)(frame->pkt_dts - video_stream->start_time) * video_time_base;
				current_frame = (int64_t)(frame_rate * current_time + 0.5);
			}
			else {
				current_frame++;
				current_time = (double)current_frame / frame_rate;
			}
			
			uint8_t *video_dst_data[4] = { NULL, NULL, NULL, NULL };
            int video_dst_linesize[4];
            ret = av_image_alloc(video_dst_data, video_dst_linesize, video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt, 1);
            if (ret < 0) { error = "could not allocate raw video buffer"; return -1; };
			av_image_copy(video_dst_data, video_dst_linesize, (const uint8_t **)(frame->data), frame->linesize, video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
            
			uint8_t *old_buf = frame_buffer->getBuffer();
            if (old_buf) av_freep(&old_buf);
			
			frame_buffer->setBuffer(video_dst_data[0]);
            frame_buffer->setBufferSize(ret);
            frame_buffer->setFrameIndex(current_frame);
		}
    }
    return decoded;
}

void DemuxBaton::Seek() {
	int ret;
	int64_t frameNumber = seek_timestamp * frame_rate;
	if (frameNumber < 0) frameNumber = 0;
	if (frameNumber >= num_frames) frameNumber = num_frames-1;
	
	int delta = 8;
	
	if (current_frame < 0) DecodeFrame();
    
	int64_t tmp_frameNumber = frameNumber - delta;
	if (tmp_frameNumber < 0) tmp_frameNumber = 0;
	double sec = (double)tmp_frameNumber * frame_time;
	int64_t time_stamp = video_stream->start_time;
	time_stamp += (int64_t)(sec / video_time_base + 0.5);
	ret = av_seek_frame(fmt_ctx, video_stream_idx, time_stamp, AVSEEK_FLAG_BACKWARD);
	if (ret < 0) { error = "could not seek video to specified frame"; return; }
	avcodec_flush_buffers(video_dec_ctx);
	
	do {
		DecodeFrame();
	} while(current_frame < frameNumber);
}


	