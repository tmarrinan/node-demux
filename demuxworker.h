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
		DemuxWorker(DemuxBaton *btn, bool cont)
			: NanAsyncWorker(NULL), baton(btn), continuous(cont) {};
		~DemuxWorker() {};
		
		void Execute();
		void HandleOKCallback();
		
		void DecodeFrame();
		int DecodePacket(int *got_frame, int cached);
		
		static void uv_DemuxTimer(uv_timer_t *req, int status);
		static void uv_DemuxTimer(uv_timer_t *req);
	private:
		DemuxBaton *baton;
		bool continuous;
};

#endif // DEMUXWORKER_H