#ifndef NODE_VIDEODEMUX_H
#define NODE_VIDEODEMUX_H

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/imgutils.h>
}
#include <string>
#include <vector>
#include <nan.h>
#include <node.h>
#include <node_buffer.h>
#include "loadworker.h"
#include "demuxworker.h"
#include "seekworker.h"



class VideoDemux : public node::ObjectWrap {
	public:
		static NAN_MODULE_INIT(Init);
	
	private:
		explicit VideoDemux();
		~VideoDemux();
	
		static NAN_METHOD(New);
		static NAN_METHOD(LoadVideo);
		static NAN_METHOD(StartDemuxing);
		static NAN_METHOD(DemuxFrame);
		static NAN_METHOD(PauseDemuxing);
		static NAN_METHOD(StopDemuxing);
		static NAN_METHOD(SeekVideo);
		static NAN_METHOD(On);
		static NAN_METHOD(IsBusy);
		
		static v8::Persistent<v8::FunctionTemplate> constructor;
		
		DemuxBaton *baton;
};


#endif // NODE_VIDEODEMUX_H
