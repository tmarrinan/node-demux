#include "demuxworker.h"

void DemuxWorker::Execute() {
	baton->state = DS_DEMUX;
	DecodeFrame();
}

void DemuxWorker::HandleOKCallback() {
	NanScope();
	
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
				baton->m_Frame(baton->frame_buffer);
				if(continuous) {
					// timer to call DemuxWorker again
					uint64_t demux_curr = uv_now(uv_default_loop());
					uint64_t video_curr = baton->current_frame * baton->frame_time * 1000.0;
					int64_t diff = (video_curr - baton->video_start) - (demux_curr - baton->demux_start);
					if (diff <= 0) NanAsyncQueueWorker(new DemuxWorker(baton, true));
					else uv_timer_start(&baton->timerReq, uv_DemuxTimer, diff, 0);
				}
				else {
					baton->action = DA_NONE;
				}
				break;
			case DA_PAUSE:
				baton->m_Frame(baton->frame_buffer);
				break;
			case DA_SEEK:
				baton->m_Frame(baton->frame_buffer);
				break;
			case DA_END:
				baton->m_End();
				break;
			default:
				break;
		}
	}
}

void DemuxWorker::DecodeFrame() {
	int ret = 0, got_frame = 0;
	
	while (true) {
		// read new packet if empty
		if (baton->pkt.size <= 0) {
            ret = av_read_frame(baton->fmt_ctx, &baton->pkt);
			if (ret < 0) break;
			baton->orig_pkt = baton->pkt;
		}
		do {
			ret = DecodePacket(&got_frame, 0);
			if (ret < 0) break;
			baton->pkt.data += ret;
			baton->pkt.size -= ret;
		} while (baton->pkt.size > 0 && !got_frame);
		if (baton->pkt.size <= 0) av_free_packet(&baton->orig_pkt);
        if (got_frame) return;
	}
	
	// flush cached frames
	baton->pkt.data = NULL;
	baton->pkt.size = 0;
	DecodePacket(&got_frame, 1);
	if (!got_frame) baton->action = DA_END;
}

int DemuxWorker::DecodePacket(int *got_frame, int cached) {
	int ret = 0;
    int decoded = baton->pkt.size;
    if (baton->pkt.stream_index == baton->video_stream_idx) {
		// decode video frame
		ret = avcodec_decode_video2(baton->video_dec_ctx, baton->frame, got_frame, &baton->pkt);
		if(ret < 0) { baton->error = "could not decode video frame"; return -1; }
		if (*got_frame) {
			//baton->new_frame = true;
			
			if (baton->frame->pkt_dts >= 0) {
				baton->current_time = (double)(baton->frame->pkt_dts - baton->video_stream->start_time) * baton->video_time_base;
				baton->current_frame = (int64_t)(baton->frame_rate * baton->current_time + 0.5);
			}
			else {
				baton->current_frame++;
				baton->current_time = (double)baton->current_frame / baton->frame_rate;
			}
			
			uint8_t *video_dst_data[4] = { NULL, NULL, NULL, NULL };
			int video_dst_linesize[4];
            ret = av_image_alloc(video_dst_data, video_dst_linesize, baton->video_dec_ctx->width, baton->video_dec_ctx->height, baton->video_dec_ctx->pix_fmt, 1);
            if (ret < 0) { baton->error = "could not allocate raw video buffer"; return -1; };
			
			av_image_copy(video_dst_data, video_dst_linesize, (const uint8_t **)(baton->frame->data), baton->frame->linesize, baton->video_dec_ctx->pix_fmt, baton->video_dec_ctx->width, baton->video_dec_ctx->height);
            
			uint8_t *old_buf = baton->frame_buffer->getBuffer();
            if (old_buf) av_freep(&old_buf);
			
			baton->frame_buffer->setBuffer(video_dst_data[0]);
			baton->frame_buffer->setBufferSize(ret);
			baton->frame_buffer->setFrameIndex(baton->current_frame);
		}
    }
    return decoded;
}

void DemuxWorker::uv_DemuxTimer(uv_timer_t *req, int status) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	NanAsyncQueueWorker(new DemuxWorker(btn, true));
}

void DemuxWorker::uv_DemuxTimer(uv_timer_t *req) {
	DemuxBaton *btn = static_cast<DemuxBaton *>(req->data);
	
	NanAsyncQueueWorker(new DemuxWorker(btn, true));
}