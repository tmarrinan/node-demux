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
	