#include "demuxworker.h"

void DemuxWorker::Execute() {
	printf("thread executing %lf\n", baton->duration);
}

void DemuxWorker::HandleOKCallback() {
	NanScope();
	
	printf("finished thread\n");
}
