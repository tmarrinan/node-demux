#ifndef LOADWORKER_H
#define LOADWORKER_H

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

class LoadWorker : public NanAsyncWorker {
	public:
		LoadWorker(DemuxBaton *btn, std::string fn, bool dff)
			: NanAsyncWorker(NULL), baton(btn), filename(fn), decodeFirstFrame(dff) {};
		~LoadWorker() {};
		
		void Execute();
		void HandleOKCallback();
		
		void OpenVideoFile();
		int OpenCodecContext(int *stream_idx, AVFormatContext *fctx);
		
	private:
		DemuxBaton *baton;
		std::string filename;
		bool decodeFirstFrame;
};

#endif // LOADWORKER_H