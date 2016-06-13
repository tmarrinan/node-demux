#include "loadworker.h"

void LoadWorker::Execute() {
	baton->state = DS_LOAD;
	baton->filename = filename;
	baton->decode_first_frame = decodeFirstFrame;
	baton->colorspace = colorspace;
	baton->OpenVideoFile();
}

void LoadWorker::HandleOKCallback() {
	Nan::HandleScope();
	
	if(baton->error != "") {
		baton->m_Error(baton->error);
	}
	else {
		baton->m_MetaData();
		baton->state = DS_IDLE;
		switch (baton->action) {
			case DA_LOAD:
				baton->action = DA_NONE;
				if(baton->decode_first_frame) {
					Nan::AsyncQueueWorker(new DemuxWorker(baton, false));
				}
				break;
			case DA_PLAY:
				baton->demux_start = uv_now(uv_default_loop());
				baton->video_start = baton->current_frame * baton->frame_time * 1000.0;
				baton->m_Start();
				Nan::AsyncQueueWorker(new DemuxWorker(baton, true));
				break;
			case DA_NEXT_FRAME:
				baton->demux_start = uv_now(uv_default_loop());
				baton->video_start = baton->current_frame * baton->frame_time * 1000.0;
				baton->m_Start();
				Nan::AsyncQueueWorker(new DemuxWorker(baton, false));
				break;
			case DA_PAUSE:
				baton->action = DA_NONE;
				baton->m_Pause();
				break;
			case DA_SEEK:
				Nan::AsyncQueueWorker(new SeekWorker(baton));
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
