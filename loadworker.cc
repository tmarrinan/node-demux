#include "loadworker.h"

void LoadWorker::Execute() {
	baton->state = DS_LOAD;
	baton->filename = filename;
	baton->decode_first_frame = decodeFirstFrame;
	baton->OpenVideoFile();
}

void LoadWorker::HandleOKCallback() {
	NanScope();
	
	if(baton->error != "") {
		baton->m_Error(baton->error);
	}
	else {
		baton->m_MetaData();
		baton->state = DS_IDLE;
		switch (baton->action) {
			case DA_LOAD:
				if(baton->decode_first_frame) {
					NanAsyncQueueWorker(new DemuxWorker(baton, false));
				}
				baton->action = DA_NONE;
				break;
			case DA_PLAY:
				baton->demux_start = uv_now(uv_default_loop());
				baton->video_start = baton->current_frame * baton->frame_time * 1000.0;
				baton->m_Start();
				NanAsyncQueueWorker(new DemuxWorker(baton, true));
				break;
			case DA_PAUSE:
				baton->action = DA_NONE;
				break;
			case DA_SEEK:
				NanAsyncQueueWorker(new SeekWorker(baton));
				break;
			case DA_END:
				baton->m_End();
				break;
			default:
				break;
		}
	}
}
