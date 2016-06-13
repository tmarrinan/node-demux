#include "demuxworker.h"

void DemuxWorker::Execute() {
	baton->state = DS_DEMUX;
	baton->DecodeFrame();
}

void DemuxWorker::HandleOKCallback() {
	Nan::HandleScope();
	
	if(baton->error != "") {
		baton->m_Error(baton->error);
	}
	else {
		baton->state = DS_IDLE;
		switch (baton->action) {
			case DA_NONE:
				baton->m_Frame(baton->frame_buffer);
				break;
			case DA_LOAD:
				break;
			case DA_PLAY:
				if(continuous) {
					baton->state = DS_DEMUX;
					uint64_t demux_curr = uv_now(uv_default_loop());
					uint64_t video_curr = baton->current_frame * baton->frame_time * 1000.0;
					int64_t diff = (video_curr - baton->video_start) - (demux_curr - baton->demux_start);
					if (diff <= 0) {
						Nan::AsyncQueueWorker(new DemuxWorker(baton, true));
					}
					else {
						baton->m_Frame(baton->frame_buffer);
						uv_timer_start(&baton->timerReq, uv_DemuxTimer, diff, 0);
					}
				}
				else {
					baton->action = DA_NONE;
					baton->m_Frame(baton->frame_buffer);
				}
				break;
			case DA_NEXT_FRAME:
				baton->m_Frame(baton->frame_buffer);
				baton->action = DA_NONE;
				break;
			case DA_PAUSE:
				baton->action = DA_NONE;
				baton->m_Frame(baton->frame_buffer);
				baton->m_Pause();
				break;
			case DA_SEEK:
				baton->m_Frame(baton->frame_buffer);
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

void DemuxWorker::uv_DemuxTimer(uv_timer_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	Nan::AsyncQueueWorker(new DemuxWorker(btn, true));
}

void DemuxWorker::uv_DemuxTimer(uv_timer_t *req) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	Nan::AsyncQueueWorker(new DemuxWorker(btn, true));
}