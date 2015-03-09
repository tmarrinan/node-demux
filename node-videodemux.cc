#include <node.h>
#include "node-videodemux.h"

using namespace v8;


Persistent<FunctionTemplate> VideoDemux::constructor;

VideoDemux::VideoDemux() {
	baton = new DemuxBaton();
}

VideoDemux::~VideoDemux() {

}

void VideoDemux::Init(Handle<Object> exports) {
	NanScope();
	
	// Prepare constructor template
	Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(VideoDemux::New);
	tpl->SetClassName(NanNew("VideoDemux"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	NODE_SET_PROTOTYPE_METHOD(tpl, "LoadVideo",     LoadVideo);
	NODE_SET_PROTOTYPE_METHOD(tpl, "StartDemuxing", StartDemuxing);
	NODE_SET_PROTOTYPE_METHOD(tpl, "PauseDemuxing", PauseDemuxing);
	NODE_SET_PROTOTYPE_METHOD(tpl, "StopDemuxing",  StopDemuxing);
	NODE_SET_PROTOTYPE_METHOD(tpl, "SeekVideo",     SeekVideo);
	NODE_SET_PROTOTYPE_METHOD(tpl, "On",            On);
	NODE_SET_PROTOTYPE_METHOD(tpl, "IsBusy",        IsBusy);
	
	exports->Set(NanNew<String>("VideoDemux"), tpl->GetFunction());
	NanAssignPersistent(constructor, tpl);
}


/**********************************************************************************/
NAN_METHOD(VideoDemux::New) {
	NanScope();

	if (!args.IsConstructCall()) {
		NanThrowError("VideoDemux::New - Cannot call constructor as function, you need to use 'new' keyword");
		NanReturnUndefined();
	}
	else {
		// Invoked as constructor: `new VideoDemux(...)`
		VideoDemux *obj = new VideoDemux();
		obj->Wrap(args.This());
		NanReturnValue(args.This());
	}
}

NAN_METHOD(VideoDemux::LoadVideo) {
	NanScope();
	
	if (args.Length() < 1) {
		NanThrowError("VideoDemux::LoadVideo - Wrong number of arguments");
		NanReturnUndefined();
	}
	
	bool dff = false;
	if (args.Length() >= 2) {
		Local<Object> obj = args[1]->ToObject();
		if (obj->Has(NanNew<String>("decodeFirstFrame"))) {
			dff = obj->Get(NanNew<String>("decodeFirstFrame"))->BooleanValue();
		}
	}
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->baton->action = DA_LOAD;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_LOAD;
		NanAsyncQueueWorker(new LoadWorker(obj->baton, *NanUtf8String(args[0]), dff));
	}
	
	NanReturnUndefined();
}

NAN_METHOD(VideoDemux::StartDemuxing) {
	NanScope();
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->baton->action = DA_PLAY;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_DEMUX;
		obj->baton->demux_start = uv_now(uv_default_loop());
		obj->baton->video_start = obj->baton->current_frame * obj->baton->frame_time * 1000.0;
		obj->baton->m_Start();
		NanAsyncQueueWorker(new DemuxWorker(obj->baton, true));
	}
	
	NanReturnUndefined();
}

NAN_METHOD(VideoDemux::PauseDemuxing) {
	NanScope();
	
	if(args.Length() < 1) {
		NanThrowError("VideoDemux::PauseVideo - Wrong number of arguments");
		NanReturnUndefined();
	}
	
	NanCallback *callback = new NanCallback(args[0].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->baton->action = DA_PAUSE;
	obj->baton->PauseCallback = callback;
	
	NanReturnUndefined();
}

NAN_METHOD(VideoDemux::StopDemuxing) {
	NanScope();
	
	if(args.Length() < 1) {
		NanThrowError("VideoDemux::StopVideo - Wrong number of arguments");
		NanReturnUndefined();
	}
	
	double timestamp = 0.0;
	NanCallback *callback = new NanCallback(args[0].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->baton->action = DA_SEEK;
	obj->baton->seek_timestamp = timestamp;
	obj->baton->SeekCallback = callback;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_SEEK;
		NanAsyncQueueWorker(new SeekWorker(obj->baton));
	}
	
	NanReturnUndefined();
}

NAN_METHOD(VideoDemux::SeekVideo) {
	NanScope();
	
	if(args.Length() < 2) {
		NanThrowError("VideoDemux::SeekVideo - Wrong number of arguments");
		NanReturnUndefined();
	}
	
	double timestamp = args[0]->NumberValue();
	NanCallback *callback = new NanCallback(args[1].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	obj->baton->action = DA_SEEK;
	obj->baton->seek_timestamp = timestamp;
	obj->baton->SeekCallback = callback;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_SEEK;
		NanAsyncQueueWorker(new SeekWorker(obj->baton));
	}
	
	NanReturnUndefined();
}

NAN_METHOD(VideoDemux::On) {
	NanScope();
	
	if(args.Length() < 2) {
		NanThrowError("VideoDemux::On - Wrong number of arguments");
		NanReturnUndefined();
	}
	
	std::string type = *NanUtf8String(args[0]);
	NanCallback *callback = new NanCallback(args[1].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(args.This());
	if (type == "error") {
		obj->baton->def_err = true;
		obj->baton->OnError = callback;
	}
	else if (type == "metadata") {
		obj->baton->def_meta = true;
		obj->baton->OnMetaData = callback;
	}
	else if (type == "start") {
		obj->baton->def_start = true;
		obj->baton->OnStart = callback;
	}
	else if (type == "end") {
		obj->baton->def_end = true;
		obj->baton->OnEnd = callback;
	}
	else if (type == "frame") {
		obj->baton->def_frame = true;
		obj->baton->OnFrame = callback;
	}
	
	NanReturnUndefined();
}

NAN_METHOD(VideoDemux::IsBusy) {
	NanScope();
	
	NanReturnUndefined();
}	
