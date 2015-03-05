#ifndef SEEKWORKER_H
#define SEEKWORKER_H

#include <node.h>
#include <nan.h>
#include "demuxbaton.h"
#include "demuxworker.h"

class SeekWorker : public NanAsyncWorker {
	public:
		SeekWorker(DemuxBaton *btn)
			: NanAsyncWorker(NULL), baton(btn) {};
		~SeekWorker() {};
		
		void Execute();
		void HandleOKCallback();
		
	private:
		DemuxBaton *baton;
};

#endif // SEEKWORKER_H