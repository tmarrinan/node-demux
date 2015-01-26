#include <node.h>
#include "node-videodemux.h"

using namespace v8;


Persistent<Function> VideoDemux::constructor;

VideoDemux::VideoDemux() {
	baton = new DemuxBaton();
	baton->fmt_ctx           = NULL;
	baton->video_dec_ctx     = NULL;
	baton->video_stream      = NULL;
	baton->frame             = NULL;
	baton->video_stream_idx  = -1;
	baton->frame_buffer = new VideoFrame();
	baton->finished = false;
	baton->error = "";
	
	baton->def_err   = false;
	baton->def_start = false;
	baton->def_end   = false;
	baton->def_frame = false;
	
	baton->busy = false;
	baton->seek_when_ready = false;
	
	baton->NodeBuffer = Persistent<Function>::New(Handle<Function>::Cast(Context::GetCurrent()->Global()->Get(String::New("Buffer"))));
}

VideoDemux::~VideoDemux() {

}

void VideoDemux::m_MetaData(DemuxBaton *btn, int width, int height, int64_t num_frames, double frame_rate, double duration, std::string format) {
	if (btn->def_meta) {
		Local<Object> meta = Object::New();
		meta->Set(String::NewSymbol("width"),        Number::New(width));
		meta->Set(String::NewSymbol("height"),       Number::New(height));
		meta->Set(String::NewSymbol("num_frames"),   Number::New(num_frames));
		meta->Set(String::NewSymbol("frame_rate"),   Number::New(frame_rate));
		meta->Set(String::NewSymbol("duration"),     Number::New(duration));
		meta->Set(String::NewSymbol("pixel_format"), String::New(format.c_str()));
		Local<Value> argv[1];
		argv[0] = Local<Value>::New(meta);
		btn->OnMetaData->Call(Context::GetCurrent()->Global(), 1, argv);
	}
};

void VideoDemux::m_Error(DemuxBaton *btn, std::string msg) {
	HandleScope scope;
	if (btn->def_err) {
		Local<Value> argv[1] = { Local<Value>::New(String::New(msg.c_str())) };
		btn->OnError->Call(Context::GetCurrent()->Global(), 1, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::m_Start(DemuxBaton *btn) {
	HandleScope scope;
	if (btn->def_start) {
		Local<Value> argv[1];
		btn->OnStart->Call(Context::GetCurrent()->Global(), 0, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::m_End(DemuxBaton *btn) {
	HandleScope scope;
	if (btn->def_end) {
		Local<Value> argv[1];
		btn->OnEnd->Call(Context::GetCurrent()->Global(), 0, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::m_Pause(DemuxBaton *btn) {
	HandleScope scope;
	
	Local<Value> argv[1];
	btn->callback->Call(Context::GetCurrent()->Global(), 0, argv);
	
	scope.Close(Undefined());
}

void VideoDemux::m_Seek(DemuxBaton *btn) {
	HandleScope scope;
	
	Local<Value> argv[1];
	btn->callback->Call(Context::GetCurrent()->Global(), 0, argv);
	
	scope.Close(Undefined());
}

void VideoDemux::m_Frame(DemuxBaton *btn, VideoFrame *frm) {
	HandleScope scope;
	if (btn->def_frame) {
		size_t size = frm->getBufferSize();
		uint8_t *buf = frm->getBuffer();
		int64_t frameIdx = frm->getFrameIndex();
		
		node::Buffer *slowbuf = node::Buffer::New(size);
		memcpy(node::Buffer::Data(slowbuf), buf, size);
		
		Handle<Value> bufArgs[3] = { slowbuf->handle_, Integer::New(size), Integer::New(0) };
		Local<Object> buffer = btn->NodeBuffer->NewInstance(3, bufArgs);
	
		Local<Value> argv[2];
		argv[0] = Local<Value>::New(Number::New(frameIdx));
		argv[1] = Local<Value>::New(buffer);
		btn->OnFrame->Call(Context::GetCurrent()->Global(), 2, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::m_LoadVideo(std::string fn) {
	int ret = 0;
	baton->filename = fn;
	
	// open input file, and allocate format context
	ret = avformat_open_input(&baton->fmt_ctx, baton->filename.c_str(), NULL, NULL);
	if (ret < 0) { m_Error(baton, "could not open source file: " + baton->filename); return; }
	ret = avformat_find_stream_info(baton->fmt_ctx, NULL);
	if (ret < 0) { m_Error(baton, "could not find stream information"); return; }
	ret = m_OpenCodecContext(&baton->video_stream_idx, baton->fmt_ctx);
	if (ret < 0) { return; }
	baton->video_stream = baton->fmt_ctx->streams[baton->video_stream_idx];
	baton->video_dec_ctx = baton->video_stream->codec;
	
	// get video metadata
	baton->width  = baton->video_dec_ctx->width;
	baton->height = baton->video_dec_ctx->height;
	baton->num_frames = baton->video_stream->nb_frames;
	baton->duration = (double)baton->fmt_ctx->duration / (double)AV_TIME_BASE;
	baton->frame_rate = (double)baton->video_stream->avg_frame_rate.num/(double)baton->video_stream->avg_frame_rate.den;
	baton->frame_time = 1.0 / baton->frame_rate;
	baton->video_time_base = (double)baton->video_stream->time_base.num / (double)baton->video_stream->time_base.den;
	if(baton->num_frames == 0) baton->num_frames = (int64_t)floor((baton->duration * baton->frame_rate) + 0.5);
	
	if      (baton->video_dec_ctx->pix_fmt == PIX_FMT_YUV420P) baton->format = "yuv420p";
	else if (baton->video_dec_ctx->pix_fmt == PIX_FMT_RGB24)   baton->format = "rgb24";
	else if (baton->video_dec_ctx->pix_fmt == PIX_FMT_RGB32)   baton->format = "rgb32";
	else                                                       baton->format = "unknown";
	m_MetaData(baton, baton->width, baton->height, baton->num_frames, baton->frame_rate, baton->duration, baton->format);
	
	baton->frame = av_frame_alloc();
	if (!baton->frame) { m_Error(baton, "could not allocate frame"); return; }
    
    baton->pkt.data = NULL;
	baton->pkt.size = 0;
	
	baton->paused = true;
	baton->new_frame = false;
	baton->cue_in_frame = -1;
	baton->current_frame= -1;
	baton->workDemuxReq.data = baton;
	baton->workSeekReq.data = baton;
	baton->timerReq.data = baton;
	av_init_packet(&baton->pkt);
	uv_timer_init(uv_default_loop(), &baton->timerReq);
}

void VideoDemux::m_StartDemuxing() {
	baton->dem_start = uv_now(uv_default_loop());
	baton->vid_start = baton->current_frame * baton->frame_time * 1000.0;
	baton->paused = false;
	m_Start(baton);
	
	uv_queue_work(uv_default_loop(), &baton->workDemuxReq, uv_DemuxAsync, uv_DemuxAsyncAfter);
}

void VideoDemux::m_PauseDemuxing(Persistent<Function> callback) {
	if(baton->paused) {
		baton->callback = callback;
		m_Pause(baton);
		return;
	}
	
	baton->busy = true;
	
	baton->paused = true;
	baton->callback = callback;
}

void VideoDemux::m_StopDemuxing(Persistent<Function> callback) {
	m_SeekVideo(0.0, callback);
}

void VideoDemux::m_SeekVideo(double timestamp, Persistent<Function> callback) {
	baton->busy = true;
	
	baton->seek_timestamp = timestamp;
	baton->callback = callback;
	baton->finished = false;
	
	if(baton->paused) uv_queue_work(uv_default_loop(), &baton->workSeekReq, uv_SeekAsync, uv_SeekAsyncAfter);
	else { baton->paused = true; baton->seek_when_ready = true; }
}

void VideoDemux::m_DecodeFrame(DemuxBaton *btn) {
	int ret = 0, got_frame = 0;
	
	while (true) {
		// read new packet if empty
		if (btn->pkt.size <= 0) {
            ret = av_read_frame(btn->fmt_ctx, &btn->pkt);
			if (ret < 0) break;
			btn->orig_pkt = btn->pkt;
		}
		do {
			ret = m_DecodePacket(btn, &got_frame, 0);
			if (ret < 0) break;
			btn->pkt.data += ret;
			btn->pkt.size -= ret;
		} while (btn->pkt.size > 0 && !got_frame);
		if (btn->pkt.size <= 0) av_free_packet(&btn->orig_pkt);
        if (got_frame) return;
	}
	
	// flush cached frames
	btn->pkt.data = NULL;
	btn->pkt.size = 0;
	m_DecodePacket(btn, &got_frame, 1);
	if (!got_frame) btn->finished = true;
}

void VideoDemux::uv_DemuxTimer(uv_timer_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	uv_queue_work(uv_default_loop(), &btn->workDemuxReq, uv_DemuxAsync, uv_DemuxAsyncAfter);
}

void VideoDemux::uv_SeekAsync(uv_work_t *req) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	int ret;
	int64_t frameNumber = btn->seek_timestamp * btn->frame_rate;
	if (frameNumber < 0) frameNumber = 0;
	if (frameNumber > btn->num_frames) frameNumber = btn->num_frames;
	
	int delta = 8;
	
	if (btn->current_frame < 0) m_DecodeFrame(btn);
    
	int64_t tmp_frameNumber = frameNumber - delta;
	if (tmp_frameNumber < 0) tmp_frameNumber = 0;
	double sec = (double)tmp_frameNumber * btn->frame_time;
	int64_t time_stamp = btn->video_stream->start_time;
	time_stamp += (int64_t)(sec / btn->video_time_base + 0.5);
	ret = av_seek_frame(btn->fmt_ctx, btn->video_stream_idx, time_stamp, AVSEEK_FLAG_BACKWARD);
	if (ret < 0) { btn->error = "could not seek video to specified frame"; return; }
	avcodec_flush_buffers(btn->video_dec_ctx);
	
	do {
		m_DecodeFrame(btn);
	} while(btn->current_frame < frameNumber);
	btn->new_frame = true;
}

void VideoDemux::uv_SeekAsyncAfter(uv_work_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	btn->busy = false;
	m_Seek(btn);
}

void VideoDemux::uv_DemuxAsync(uv_work_t *req) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	if (!btn->new_frame) m_DecodeFrame(btn);
}

void VideoDemux::uv_DemuxAsyncAfter(uv_work_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	if (btn->error != "") {
		m_Error(btn, btn->error);
		btn->error = "";
		return;
	}
	
	m_Frame(btn, btn->frame_buffer);
	btn->new_frame = false;
	
	if(btn->paused) {
		if(btn->seek_when_ready) {
			uv_queue_work(uv_default_loop(), &btn->workSeekReq, uv_SeekAsync, uv_SeekAsyncAfter);
			btn->seek_when_ready = false;
		}
		else {
			btn->busy = false;
			m_Pause(btn);
		}
	}
	
	if (btn->finished) {
		btn->paused = true;
		m_End(btn);
	}
	else if (!btn->paused) {
		uint64_t dem_curr = uv_now(uv_default_loop());
		uint64_t vid_curr = btn->current_frame * btn->frame_time * 1000.0;
		int64_t diff = (vid_curr - btn->vid_start) - (dem_curr - btn->dem_start);
		if (diff <= 0) uv_queue_work(uv_default_loop(), &btn->workDemuxReq, uv_DemuxAsync, uv_DemuxAsyncAfter);
		else uv_timer_start(&btn->timerReq, uv_DemuxTimer, diff, 0);
	}
}

int VideoDemux::m_OpenCodecContext(int *stream_idx, AVFormatContext *fctx) {
    int ret;
    AVStream *st;
    AVCodecContext *cctx = NULL;
    AVCodec *codec = NULL;
    ret = av_find_best_stream(fctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) { m_Error(baton, "could not find video stream in input file"); return -1; }
    
	*stream_idx = ret;
	st = fctx->streams[*stream_idx];
	// find decoder for the stream
	cctx = st->codec;
	codec = avcodec_find_decoder(cctx->codec_id);
	if (!codec) { m_Error(baton, "failed to find codec"); return -1; };
	ret = avcodec_open2(cctx, codec, NULL);
	if (ret < 0) { m_Error(baton, "failed to open codec"); return -1; }
	
    return 0;
}

int VideoDemux::m_DecodePacket(DemuxBaton *btn, int *got_frame, int cached) {
	int ret = 0;
    int decoded = btn->pkt.size;
    if (btn->pkt.stream_index == btn->video_stream_idx) {
		// decode video frame
		ret = avcodec_decode_video2(btn->video_dec_ctx, btn->frame, got_frame, &btn->pkt);
		if(ret < 0) { btn->error = "could not decode video frame"; return -1; }
		if (*got_frame) {
			btn->new_frame = true;
			
			if (btn->frame->pkt_dts >= 0) {
				btn->current_time = (double)(btn->frame->pkt_dts - btn->video_stream->start_time) * btn->video_time_base;
				btn->current_frame = (int64_t)(btn->frame_rate * btn->current_time + 0.5);
			}
			else {
				btn->current_frame++;
				btn->current_time = (double)btn->current_frame / btn->frame_rate;
			}
			
			uint8_t *video_dst_data[4] = { NULL, NULL, NULL, NULL };
			int video_dst_linesize[4];
            ret = av_image_alloc(video_dst_data, video_dst_linesize, btn->video_dec_ctx->width, btn->video_dec_ctx->height, btn->video_dec_ctx->pix_fmt, 1);
            if (ret < 0) { btn->error = "could not allocate raw video buffer"; return -1; };
			
			av_image_copy(video_dst_data, video_dst_linesize, (const uint8_t **)(btn->frame->data), btn->frame->linesize, btn->video_dec_ctx->pix_fmt, btn->video_dec_ctx->width, btn->video_dec_ctx->height);
            
			uint8_t *old_buf = btn->frame_buffer->getBuffer();
            if (old_buf) av_freep(&old_buf);
			
			btn->frame_buffer->setBuffer(video_dst_data[0]);
			btn->frame_buffer->setBufferSize(ret);
			btn->frame_buffer->setFrameIndex(btn->current_frame);
		}
    }
    return decoded;
}

void VideoDemux::m_On(std::string type, Persistent<Function> callback) {
	if      (type == "error")
		{ baton->def_err   = true; baton->OnError    = callback; }
	else if (type == "metadata")
		{ baton->def_meta  = true; baton->OnMetaData = callback; }
	else if (type == "start")
		{ baton->def_start = true; baton->OnStart    = callback; }
	else if (type == "end")
		{ baton->def_end   = true; baton->OnEnd      = callback; }
	else if (type == "frame")
		{ baton->def_frame = true; baton->OnFrame    = callback; }
}


void VideoDemux::Init(Handle<Object> exports) {
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("VideoDemux"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadVideo"), FunctionTemplate::New(LoadVideo)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("StartDemuxing"), FunctionTemplate::New(StartDemuxing)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("PauseDemuxing"), FunctionTemplate::New(PauseDemuxing)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("StopDemuxing"), FunctionTemplate::New(StopDemuxing)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SeekVideo"), FunctionTemplate::New(SeekVideo)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("On"), FunctionTemplate::New(On)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("IsBusy"), FunctionTemplate::New(IsBusy)->GetFunction());
	constructor = Persistent<Function>::New(tpl->GetFunction());
	exports->Set(String::NewSymbol("VideoDemux"), constructor);
}

Handle<Value> VideoDemux::New(const Arguments& args) {
	HandleScope scope;

	if (args.IsConstructCall()) {
		// Invoked as constructor: `new VideoDemux(...)`
		VideoDemux *obj = new VideoDemux();
		obj->Wrap(args.This());
		return args.This();
	} else {
		// Invoked as plain function `VideoDemux(...)`, turn into construct call.
		Local<Value> argv[1];
		return scope.Close(constructor->NewInstance(0, argv));
	}
}

Handle<Value> VideoDemux::LoadVideo(const Arguments& args) {
	HandleScope scope;
	
	if(args.Length() < 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->m_LoadVideo(*String::AsciiValue(args[0]->ToString()));
	
	return scope.Close(Undefined());
}

Handle<Value> VideoDemux::StartDemuxing(const Arguments& args) {
	HandleScope scope;
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->m_StartDemuxing();
	
	return scope.Close(Undefined());
}

Handle<Value> VideoDemux::PauseDemuxing(const Arguments& args) {
	HandleScope scope;
	
	if(args.Length() < 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	Persistent<Function> callback = Persistent<Function>::New(Handle<Function>::Cast(args[0]));
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->m_PauseDemuxing(callback);
	
	return scope.Close(Undefined());
}

Handle<Value> VideoDemux::StopDemuxing(const Arguments& args) {
	HandleScope scope;
	
	if(args.Length() < 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	Persistent<Function> callback = Persistent<Function>::New(Handle<Function>::Cast(args[0]));
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->m_StopDemuxing(callback);
	
	return scope.Close(Undefined());
}

Handle<Value> VideoDemux::SeekVideo(const Arguments& args) {
	HandleScope scope;
	
	if(args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	double timestamp = args[0]->ToNumber()->NumberValue();
	Persistent<Function> callback = Persistent<Function>::New(Handle<Function>::Cast(args[1]));
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->m_SeekVideo(timestamp, callback);
	
	return scope.Close(Undefined());
}

Handle<Value> VideoDemux::On(const Arguments& args) {
	HandleScope scope;
	
	if(args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	std::string type = *String::AsciiValue(args[0]->ToString());
	Persistent<Function> callback = Persistent<Function>::New(Handle<Function>::Cast(args[1]));
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->m_On(type, callback);
	
	return scope.Close(Undefined());
}

Handle<Value> VideoDemux::IsBusy(const Arguments& args) {
	HandleScope scope;
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	
	return scope.Close(Boolean::New(obj->baton->busy));
}
	