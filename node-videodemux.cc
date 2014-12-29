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
	baton->video_frame_count = 0;
	baton->finished = false;
	
	baton->def_err   = false;
	baton->def_start = false;
	baton->def_end   = false;
	baton->def_frame = false;
	
	baton->NodeBuffer = Persistent<Function>::New(Handle<Function>::Cast(Context::GetCurrent()->Global()->Get(String::New("Buffer"))));
}

VideoDemux::~VideoDemux() {

}

void VideoDemux::uv_MetaData(DemuxBaton *btn, int width, int height, int64_t num_frames, double frame_rate, double duration, std::string format) {
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

void VideoDemux::uv_Error(DemuxBaton *btn, std::string msg) {
	HandleScope scope;
	if (btn->def_err) {
		Local<Value> argv[1] = { Local<Value>::New(String::New(msg.c_str())) };
		btn->OnError->Call(Context::GetCurrent()->Global(), 1, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::uv_Start(DemuxBaton *btn) {
	HandleScope scope;
	if (btn->def_start) {
		Local<Value> argv[0] = { };
		btn->OnStart->Call(Context::GetCurrent()->Global(), 0, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::uv_End(DemuxBaton *btn) {
	HandleScope scope;
	if (btn->def_end) {
		Local<Value> argv[0] = { };
		btn->OnEnd->Call(Context::GetCurrent()->Global(), 0, argv);
	}
	scope.Close(Undefined());
}

void VideoDemux::uv_Frame(DemuxBaton *btn, VideoFrame *frm) {
	HandleScope scope;
	if (btn->def_frame) {
		size_t size = frm->getBufferSize();
		uint8_t *buf = frm->getBuffer();
		uint32_t frameIdx = frm->getFrameIndex();
		
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
	int width, height;
	int64_t num_frames;
	double duration, frame_rate;
	std::string format;
	
	int ret = 0;
	baton->filename = fn;
	
	// open input file, and allocate format context
	ret = avformat_open_input(&baton->fmt_ctx, baton->filename.c_str(), NULL, NULL);
	if (ret < 0) { uv_Error(baton, "could not open source file: " + baton->filename); return; }
	ret = avformat_find_stream_info(baton->fmt_ctx, NULL);
	if (ret < 0) { uv_Error(baton, "could not find stream information"); return; }
	ret = m_OpenCodecContext(&baton->video_stream_idx, baton->fmt_ctx);
	if (ret < 0) { return; }
	baton->video_stream = baton->fmt_ctx->streams[baton->video_stream_idx];
	baton->video_dec_ctx = baton->video_stream->codec;
	
	// get video metadata
	width  = baton->video_dec_ctx->width;
	height = baton->video_dec_ctx->height;
	num_frames = baton->video_stream->nb_frames;
	duration = baton->video_stream->duration * ((double)baton->video_stream->time_base.num/(double)baton->video_stream->time_base.den);
	frame_rate = (double)baton->video_stream->avg_frame_rate.num/(double)baton->video_stream->avg_frame_rate.den;
	
	if      (baton->video_dec_ctx->pix_fmt == PIX_FMT_YUV420P) format = "yuv420p";
	else if (baton->video_dec_ctx->pix_fmt == PIX_FMT_RGB24)   format = "rgb24";
	else if (baton->video_dec_ctx->pix_fmt == PIX_FMT_RGB32)   format = "rgb32";
	else                                                       format = "unknown";
	uv_MetaData(baton, width, height, num_frames, frame_rate, duration, format);
	
	baton->frame = av_frame_alloc();
	if (!baton->frame) { uv_Error(baton, "could not allocate frame"); return; }
}

void VideoDemux::m_StartDemuxing() {
	baton->workReq.data = baton;
	baton->idleReq.data = baton;
	
	av_init_packet(&baton->pkt);
	baton->pkt.data = NULL;
	baton->pkt.size = 0;
	
	uv_Start(baton);
    
	uv_queue_work(uv_default_loop(), &baton->workReq, uv_DemuxAsync, uv_DemuxAsyncAfter);
    uv_idle_init(uv_default_loop(), &baton->idleReq);
    uv_idle_start(&baton->idleReq, uv_IdleDemuxAsync);
}

void VideoDemux::uv_IdleDemuxAsync(uv_idle_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	size_t i;
	size_t len = btn->frame_buffers.size();
	for(i=0; i<len; i++) {
		uv_Frame(btn, btn->frame_buffers[i]);
		delete btn->frame_buffers[i];
	}
	btn->frame_buffers.erase(btn->frame_buffers.begin(), btn->frame_buffers.begin() + len);
	
	if(btn->finished) {
		uv_End(btn);
		uv_idle_stop(&btn->idleReq);
	}
}

void VideoDemux::uv_DemuxAsync(uv_work_t *req) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	int ret = 0, got_frame;
	
	// read frames from the file
	while (av_read_frame(btn->fmt_ctx, &btn->pkt) >= 0) {
		AVPacket orig_pkt = btn->pkt;
		do {
			ret = uv_DecodePacket(btn, &got_frame, 0);
			if (ret < 0) break;
			btn->pkt.data += ret;
			btn->pkt.size -= ret;
		} while (btn->pkt.size > 0);
		av_free_packet(&orig_pkt);
	}
	// flush cached frames
	btn->pkt.data = NULL;
	btn->pkt.size = 0;
	do {
		uv_DecodePacket(btn, &got_frame, 1);
	} while (got_frame);
}

void VideoDemux::uv_DemuxAsyncAfter(uv_work_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	btn->finished = true;
}

int VideoDemux::m_OpenCodecContext(int *stream_idx, AVFormatContext *fctx) {
    int ret;
    AVStream *st;
    AVCodecContext *cctx = NULL;
    AVCodec *codec = NULL;
    ret = av_find_best_stream(fctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) { uv_Error(baton, "could not find video stream in input file"); return -1; }
    
	*stream_idx = ret;
	st = fctx->streams[*stream_idx];
	// find decoder for the stream
	cctx = st->codec;
	codec = avcodec_find_decoder(cctx->codec_id);
	if (!codec) { uv_Error(baton, "failed to find codec"); return -1; };
	ret = avcodec_open2(cctx, codec, NULL);
	if (ret < 0) { uv_Error(baton, "failed to open codec"); return -1; }
	
    return 0;
}

int VideoDemux::uv_DecodePacket(DemuxBaton *btn, int *got_frame, int cached) {
	int ret = 0;
    int decoded = btn->pkt.size;
    if (btn->pkt.stream_index == btn->video_stream_idx) {
		// decode video frame
		ret = avcodec_decode_video2(btn->video_dec_ctx, btn->frame, got_frame, &btn->pkt);
		if(ret < 0) { uv_Error(btn, "could not decode video frame"); return -1; }
		if (*got_frame) {
			btn->video_frame_count++;
			
			uint8_t *video_dst_data[4] = { NULL, NULL, NULL, NULL };
			int video_dst_linesize[4];
			ret = av_image_alloc(video_dst_data, video_dst_linesize, btn->video_dec_ctx->width, btn->video_dec_ctx->height, btn->video_dec_ctx->pix_fmt, 1);
			if (ret < 0) { uv_Error(btn, "could not allocate raw video buffer"); return -1; };
			
			av_image_copy(video_dst_data, video_dst_linesize, (const uint8_t **)(btn->frame->data), btn->frame->linesize, btn->video_dec_ctx->pix_fmt, btn->video_dec_ctx->width, btn->video_dec_ctx->height);
			
			VideoFrame *frm = new VideoFrame(video_dst_data[0], ret, btn->video_frame_count);
			btn->frame_buffers.push_back(frm);
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
	tpl->PrototypeTemplate()->Set(String::NewSymbol("On"), FunctionTemplate::New(On)->GetFunction());
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
		Local<Value> argv[0] = { };
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
	