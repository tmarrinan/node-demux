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


class DemuxBaton {
	public:
		void m_Error(std::string msg);
		void m_MetaData();
		void m_Start();
	
	
		AVFormatContext *fmt_ctx;
		AVCodecContext *video_dec_ctx;
		AVStream *video_stream;
		AVFrame *frame;
		AVPacket pkt;
		AVPacket orig_pkt;
		int video_stream_idx;
		int64_t video_frame_number;
	
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
	
		bool busy;
		bool seek_when_ready;
		bool decode_first_frame;
		double seek_timestamp;
		v8::Persistent<v8::Function> callback;
	
		double current_time;
		int64_t current_frame;
		double cue_in_time;
		int64_t cue_in_frame;
	
		uint64_t dem_start;
		uint64_t vid_start;
		bool new_frame;
		bool finished;
		bool paused;
		std::string error;
	
		VideoFrame *frame_buffer;
	
		bool def_err;
		bool def_meta;
		bool def_start;
		bool def_end;
		bool def_frame;
		v8::Persistent<v8::Function> NodeBuffer;
		NanCallback *OnError;
		NanCallback *OnMetaData;
		NanCallback *OnStart;
		NanCallback *OnEnd;
		NanCallback *OnFrame;
};

#endif // DEMUXBATON_H