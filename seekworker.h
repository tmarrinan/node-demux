#ifndef SEEKWORKER_H
#define SEEKWORKER_H

#include <node.h>
#include <nan.h>
#include "demuxbaton.h"
#include "demuxworker.h"

class SeekWorker : public Nan::AsyncWorker {
	public:
		SeekWorker(DemuxBaton *btn)
			: Nan::AsyncWorker(NULL), baton(btn) {};
		~SeekWorker() {};
		
		void Execute();
		void HandleOKCallback();
		
	private:
		DemuxBaton *baton;
};

#endif // SEEKWORKER_H