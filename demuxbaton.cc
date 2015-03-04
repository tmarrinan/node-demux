#include "demuxbaton.h"

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
		unsigned char origBuf[8] = {72, 101, 108, 108, 111, 32, 164, 236};
		const char *buf = reinterpret_cast<const char*>(origBuf);
		uint32_t size = 8;
		Local<Object> buffer;
		
#if (NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION == 10)
		node::Buffer *slowbuf = node::Buffer::New(size);
		memcpy(node::Buffer::Data(slowbuf), buf, size);
		Handle<Value> bufArgs[3] = { slowbuf->handle_, NanNew<Integer>(size), NanNew<Integer>(0) };
		buffer = NodeBuffer->NewInstance(3, bufArgs);
#else
		buffer = NanNewBufferHandle(buf, size);
#endif
			
		Local<Object> meta = NanNew<Object>();
		meta->Set(NanNew<String>("width"),                NanNew<Number>(width));
		meta->Set(NanNew<String>("height"),               NanNew<Number>(height));
		meta->Set(NanNew<String>("display_aspect_ratio"), NanNew<Number>(display_aspect_ratio));
		meta->Set(NanNew<String>("num_frames"),           NanNew<Number>(num_frames));
		meta->Set(NanNew<String>("frame_rate"),           NanNew<Number>(frame_rate));
		meta->Set(NanNew<String>("duration"),             NanNew<Number>(duration));
		meta->Set(NanNew<String>("pixel_format"),         NanNew<String>(format.c_str()));
		meta->Set(NanNew<String>("test_buffer"),          buffer);
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
	