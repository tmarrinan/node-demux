#ifndef DEMUXBATON_H
#define DEMUXBATON_H

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#include <string>
#include <node.h>
#include <nan.h>
#include "videoframe.h"

using namespace v8;

enum DemuxState  { DS_IDLE, DS_LOAD, DS_DEMUX, DS_SEEK };
enum DemuxAction { DA_NONE, DA_LOAD, DA_PLAY, DA_PAUSE, DA_SEEK, DA_END };

class DemuxBaton {
	public:
		DemuxBaton();
		~DemuxBaton();
		
		// uv timer
		uv_timer_t timerReq;
		
		// js callback methods
		void m_Error(std::string msg);
		void m_MetaData();
		void m_Start();
		void m_End();
		void m_Frame(VideoFrame *frm);
		
		// libav handles
		AVFormatContext *fmt_ctx;
		AVCodecContext *video_dec_ctx;
		AVStream *video_stream;
		AVFrame *frame;
		AVPacket pkt;
		AVPacket orig_pkt;
		int video_stream_idx;
		int64_t video_frame_number;
		
		// metadata members
		std::string filename;
		int width;
		int height;
		int64_t num_frames;
		double display_aspect_ratio;
		double duration;
		double frame_rate;
		double frame_time;
		double video_time_base;
		std::string format;
		
		// flag for whether or not to decode one frame after load / seek
		bool decode_first_frame;
		
		// error message from thread
		std::string error;
		
		// timestamps for playback
		uint64_t demux_start;
		uint64_t video_start;
		
		// current demuxer position
		double current_time;
		int64_t current_frame;
		
		// current state and next action of demuxer
		DemuxState state;
		DemuxAction action;
		
		// video frame object
		VideoFrame *frame_buffer;
		
		// js callback functions
		bool def_err;
		bool def_meta;
		bool def_start;
		bool def_end;
		bool def_frame;
		NanCallback *OnError;
		NanCallback *OnMetaData;
		NanCallback *OnStart;
		NanCallback *OnEnd;
		NanCallback *OnFrame;
		
		// NodeJS Buffer creator - version 0.10.X
		v8::Persistent<v8::Function> NodeBuffer;
		
		/*
		bool busy;
		bool seek_when_ready;
		//bool decode_first_frame;
		double seek_timestamp;
		v8::Persistent<v8::Function> callback;
	
		//double current_time;
		//int64_t current_frame;
		double cue_in_time;
		int64_t cue_in_frame;
	
		//uint64_t dem_start;
		//uint64_t vid_start;
		bool new_frame;
		bool finished;
		bool paused;
		//std::string error;
		*/
};

#endif // DEMUXBATON_H