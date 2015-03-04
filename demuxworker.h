#ifndef DEMUXWORKER_H
#define DEMUXWORKER_H

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/imgutils.h>
}
#include <string>
#include <vector>
#include <node.h>
#include <nan.h>
#include "demuxbaton.h"

class DemuxWorker : public NanAsyncWorker {
	public:
		DemuxWorker(DemuxBaton *btn)
			: NanAsyncWorker(NULL), baton(btn) {};
		~DemuxWorker() {};
		
		void Execute();
		void HandleOKCallback();
	private:
		DemuxBaton *baton;
};

#endif // DEMUXWORKER_H