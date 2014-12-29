extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/log.h>
}
#include <node.h>
#include "node-videodemux.h"

using namespace v8;

void InitAll(Handle<Object> exports) {
	// initialize libav
	av_register_all();
	av_log_set_level(AV_LOG_QUIET);
	
	// initialize node c++ objects
	VideoDemux::Init(exports);
}

NODE_MODULE(node_demux, InitAll)