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
	//finished = false;
	error = "";
	
	def_err   = false;
	def_start = false;
	def_end   = false;
	def_frame = false;
	
	//busy = false;
	//seek_when_ready = false;
	
	state = DS_IDLE;
	action = DA_NONE;
	
#if (NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION == 10)
	NodeBuffer = Persistent<Function>::New(Handle<Function>::Cast(Context::GetCurrent()->Global()->Get(String::New("Buffer"))));
#endif
}

DemuxBaton::~DemuxBaton() {

}


void DemuxBaton::m_Error(std::string msg) {
	NanScope();

	if (def_err) {
		Local<Value> argv[1] = { NanNew<String>(msg.c_str()) };
		OnError->Call(1, argv);
	}
}

void DemuxBaton::m_MetaData() {
	NanScope();
	
	if (def_meta) {			
		Local<Object> meta = NanNew<Object>();
		meta->Set(NanNew<String>("width"),                NanNew<Number>(width));
		meta->Set(NanNew<String>("height"),               NanNew<Number>(height));
		meta->Set(NanNew<String>("display_aspect_ratio"), NanNew<Number>(display_aspect_ratio));
		meta->Set(NanNew<String>("num_frames"),           NanNew<Number>(num_frames));
		meta->Set(NanNew<String>("frame_rate"),           NanNew<Number>(frame_rate));
		meta->Set(NanNew<String>("duration"),             NanNew<Number>(duration));
		meta->Set(NanNew<String>("pixel_format"),         NanNew<String>(format.c_str()));
		Local<Value> argv[1] = {meta};
		OnMetaData->Call(1, argv);
	}
}

void DemuxBaton::m_Start() {
	NanScope();
	
	if (def_start) {
		Local<Value> argv[1];
		OnStart->Call(0, argv);
	}
}

void DemuxBaton::m_End() {
	NanScope();
	
	if (def_end) {
		Local<Value> argv[1];
		OnEnd->Call(0, argv);
	}
}

void DemuxBaton::m_Frame(VideoFrame *frm) {
	NanScope();
	
	if (def_frame) {
		uint32_t size = frm->getBufferSize();
		const char *buf = reinterpret_cast<const char*>(frm->getBuffer());
		int64_t frameIdx = frm->getFrameIndex();
		
		Local<Object> buffer;
#if (NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION == 10)
		node::Buffer *slowbuf = node::Buffer::New(size);
		memcpy(node::Buffer::Data(slowbuf), buf, size);
		Handle<Value> bufArgs[3] = { slowbuf->handle_, NanNew<Integer>(size), NanNew<Integer>(0) };
		buffer = NodeBuffer->NewInstance(3, bufArgs);
#else
		buffer = NanNewBufferHandle(buf, size);
#endif
		
		Local<Value> argv[2] = { NanNew<Number>(frameIdx), buffer };
		OnFrame->Call(2, argv);
	}
}
	