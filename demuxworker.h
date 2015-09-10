#ifndef DEMUXWORKER_H
#define DEMUXWORKER_H

#include <node.h>
#include <nan.h>
#include "demuxbaton.h"
#include "seekworker.h"

class DemuxWorker : public Nan::AsyncWorker {
	public:
		DemuxWorker(DemuxBaton *btn, bool cont)
			: Nan::AsyncWorker(NULL), baton(btn), continuous(cont) {};
		~DemuxWorker() {};
		
		void Execute();
		void HandleOKCallback();
		
		static void uv_DemuxTimer(uv_timer_t *req, int status);
		static void uv_DemuxTimer(uv_timer_t *req);
		
	private:
		DemuxBaton *baton;
		bool continuous;
};

#endif // DEMUXWORKER_H