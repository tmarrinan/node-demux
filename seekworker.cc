#include "seekworker.h"

void SeekWorker::Execute() {
	baton->state = DS_SEEK;
	baton->Seek();
}

void SeekWorker::HandleOKCallback() {
	NanScope();
	
	if(baton->error != "") {
		baton->m_Error(baton->error);
	}
	else {
		baton->m_Seek();
		if(baton->decode_first_frame) {
			baton->m_Frame(baton->frame_buffer);
		}
		baton->state = DS_IDLE;
		switch (baton->action) {
			case DA_NONE:
				break;
			case DA_LOAD:
				break;
			case DA_PLAY:
				baton->demux_start = uv_now(uv_default_loop());
				baton->video_start = baton->current_frame * baton->frame_time * 1000.0;
				baton->m_Start();
				NanAsyncQueueWorker(new DemuxWorker(baton, true));
				break;
			case DA_PAUSE:
				baton->action = DA_NONE;
				baton->m_Pause();
				break;
			case DA_SEEK:
				baton->action = DA_NONE;
				break;
			case DA_END:
				baton->action = DA_NONE;
				baton->m_End();
				break;
			default:
				break;
		}
	}
}
