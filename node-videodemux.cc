#include <node.h>
#include "node-videodemux.h"

using namespace v8;


Persistent<FunctionTemplate> VideoDemux::constructor;

VideoDemux::VideoDemux() {
	baton = new DemuxBaton();
}

VideoDemux::~VideoDemux() {

}

NAN_MODULE_INIT(VideoDemux::Init) {
	Nan::HandleScope();
	
	// Prepare constructor template
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(VideoDemux::New);
	tpl->SetClassName(Nan::New<String>("VideoDemux").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	Nan::SetPrototypeMethod(tpl, "LoadVideo",     LoadVideo);
	Nan::SetPrototypeMethod(tpl, "StartDemuxing", StartDemuxing);
	Nan::SetPrototypeMethod(tpl, "DemuxFrame",    DemuxFrame);
	Nan::SetPrototypeMethod(tpl, "PauseDemuxing", PauseDemuxing);
	Nan::SetPrototypeMethod(tpl, "StopDemuxing",  StopDemuxing);
	Nan::SetPrototypeMethod(tpl, "SeekVideo",     SeekVideo);
	Nan::SetPrototypeMethod(tpl, "On",            On);
	Nan::SetPrototypeMethod(tpl, "IsBusy",        IsBusy);
	
	//constructor.Reset(tpl->GetFunction());
	Nan::Set(target, Nan::New<String>("VideoDemux").ToLocalChecked(), tpl->GetFunction());
}


/**********************************************************************************/
NAN_METHOD(VideoDemux::New) {
	Nan::HandleScope();

	if (!info.IsConstructCall()) {
		Nan::ThrowError("VideoDemux::New - Cannot call constructor as function, you need to use 'new' keyword");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	else {
		// Invoked as constructor: `new VideoDemux(...)`
		VideoDemux *obj = new VideoDemux();
		obj->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
}

NAN_METHOD(VideoDemux::LoadVideo) {
	Nan::HandleScope();
	
	if (info.Length() < 1) {
		Nan::ThrowError("VideoDemux::LoadVideo - Wrong number of arguments");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	
	bool dff = false;
	std::string colorspace = "default";
	if (info.Length() >= 2) {
		Local<Object> obj = info[1]->ToObject();
		if (obj->Has(Nan::New<String>("decodeFirstFrame").ToLocalChecked())) {
			dff = obj->Get(Nan::New<String>("decodeFirstFrame").ToLocalChecked())->BooleanValue();
		}
		if (obj->Has(Nan::New<String>("colorspace").ToLocalChecked())) {
			colorspace = *Nan::Utf8String(obj->Get(Nan::New<String>("colorspace").ToLocalChecked()));
		}
	}
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
	obj->baton->action = DA_LOAD;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_LOAD;
		Nan::AsyncQueueWorker(new LoadWorker(obj->baton, *Nan::Utf8String(info[0]), dff, colorspace));
	}
	
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(VideoDemux::StartDemuxing) {
	Nan::HandleScope();
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
	obj->baton->action = DA_PLAY;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_DEMUX;
		obj->baton->demux_start = uv_now(uv_default_loop());
		obj->baton->video_start = obj->baton->current_frame * obj->baton->frame_time * 1000.0;
		obj->baton->m_Start();
		Nan::AsyncQueueWorker(new DemuxWorker(obj->baton, true));
	}
	
	info.GetReturnValue().Set(Nan::Undefined());
}


NAN_METHOD(VideoDemux::DemuxFrame) {
	Nan::HandleScope();
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
	obj->baton->action = DA_NEXT_FRAME;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_DEMUX;
		obj->baton->demux_start = uv_now(uv_default_loop());
		obj->baton->video_start = obj->baton->current_frame * obj->baton->frame_time * 1000.0;
		obj->baton->m_Start();
		Nan::AsyncQueueWorker(new DemuxWorker(obj->baton, true));
	}
	
	info.GetReturnValue().Set(Nan::Undefined());
}


NAN_METHOD(VideoDemux::PauseDemuxing) {
	Nan::HandleScope();
	
	if(info.Length() < 1) {
		Nan::ThrowError("VideoDemux::PauseVideo - Wrong number of arguments");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	
	Nan::Callback *callback = new Nan::Callback(info[0].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
	obj->baton->action = DA_PAUSE;
	obj->baton->PauseCallback = callback;
	
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(VideoDemux::StopDemuxing) {
	Nan::HandleScope();
	
	if(info.Length() < 1) {
		Nan::ThrowError("VideoDemux::StopVideo - Wrong number of arguments");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	
	double timestamp = 0.0;
	Nan::Callback *callback = new Nan::Callback(info[0].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
	obj->baton->action = DA_SEEK;
	obj->baton->seek_timestamp = timestamp;
	obj->baton->SeekCallback = callback;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_SEEK;
		Nan::AsyncQueueWorker(new SeekWorker(obj->baton));
	}
	
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(VideoDemux::SeekVideo) {
	Nan::HandleScope();
	
	if(info.Length() < 2) {
		Nan::ThrowError("VideoDemux::SeekVideo - Wrong number of arguments");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	
	double timestamp = info[0]->NumberValue();
	Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
	obj->baton->action = DA_SEEK;
	obj->baton->seek_timestamp = timestamp;
	obj->baton->SeekCallback = callback;
	if(obj->baton->state == DS_IDLE) {
        obj->baton->state = DS_SEEK;
		Nan::AsyncQueueWorker(new SeekWorker(obj->baton));
	}
	
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(VideoDemux::On) {
	Nan::HandleScope();
	
	if(info.Length() < 2) {
		Nan::ThrowError("VideoDemux::On - Wrong number of arguments");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	
	std::string type = *Nan::Utf8String(info[0]);
	//Nan::Persistent<Function> callback(info[1].As<Function>());
	Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
	
	VideoDemux *obj = ObjectWrap::Unwrap<VideoDemux>(info.This());
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
	
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(VideoDemux::IsBusy) {
	Nan::HandleScope();
	
	info.GetReturnValue().Set(Nan::Undefined());
}	
