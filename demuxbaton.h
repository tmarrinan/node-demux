#ifndef DEMUXBATON_H
#define DEMUXBATON_H

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/pixfmt.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
}
#include <string>
#include <node.h>
#include <nan.h>
#include "videoframe.h"

using namespace v8;

enum DemuxState  { DS_IDLE, DS_LOAD, DS_DEMUX, DS_SEEK };
enum DemuxAction { DA_NONE, DA_LOAD, DA_PLAY, DA_NEXT_FRAME, DA_PAUSE, DA_SEEK, DA_END };

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
		void m_Pause();
		void m_Seek();
		
		// demux methods
		void OpenVideoFile();
		int OpenCodecContext(int *stream_idx, AVFormatContext *fctx);
		void DecodeFrame();
		int DecodePacket(int *got_frame, int cached);
		void Seek();
		
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

		// desired colorspace for frames: 'yuv420p' or 'rgb888' (or 'default' -> do not convert)
		std::string colorspace;
		
		// actual colorspace of frames
		AVPixelFormat src_pix_fmt;

		// scale context and output image buffer (if colorspace conversion is needed)
		struct SwsContext *img_convert_ctx;
		uint8_t *output_buffer;
		
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
		
		// variables for seeking video
		double seek_timestamp;
		Nan::Callback *SeekCallback;
		
		// variables for pausing video
		Nan::Callback *PauseCallback;
		
		// js callback functions
		bool def_err;
		bool def_meta;
		bool def_start;
		bool def_end;
		bool def_frame;
		Nan::Callback *OnError;
		Nan::Callback *OnMetaData;
		Nan::Callback *OnStart;
		Nan::Callback *OnEnd;
		Nan::Callback *OnFrame;
};

#endif // DEMUXBATON_H