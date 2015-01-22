#ifndef VIDEODEMUX_H
#define VIDEODEMUX_H

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/imgutils.h>
}
#include <string>
#include <vector>
#include <node.h>
#include <node_buffer.h>


class VideoFrame {
	public:
		VideoFrame() {
			buf = NULL;
			size = 0;
			frame_idx = 0;
		};
		VideoFrame(uint8_t *b, size_t s, uint32_t f) {
			buf = b;
			size = s;
			frame_idx = f;
		};
		~VideoFrame() {
			if(buf) delete[] buf;
		};
		uint8_t* getBuffer() {
			return buf;
		}
		size_t getBufferSize() {
			return size;
		}
		int64_t getFrameIndex() {
			return frame_idx;
		}
		void setBuffer(uint8_t *b) {
			buf = b;
		}
		void setBufferSize(size_t s) {
			size = s;
		}
		void setFrameIndex(int64_t f) {
			frame_idx = f;
		}
		
	private:
		uint8_t *buf;
		size_t size;
		int64_t frame_idx;
};

struct DemuxBaton {
	uv_work_t workDemuxReq;
	uv_work_t workSeekReq;
	uv_timer_t timerReq;
	
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
	double duration;
	double frame_rate;
	double frame_time;
	double video_time_base;
	std::string format;
	
	double seek_timestamp;
	v8::Persistent<v8::Function> seek_callback;
	
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
	v8::Persistent<v8::Function> OnError;
	v8::Persistent<v8::Function> OnMetaData;
	v8::Persistent<v8::Function> OnStart;
	v8::Persistent<v8::Function> OnEnd;
	v8::Persistent<v8::Function> OnFrame;
};

class VideoDemux : public node::ObjectWrap {
	public:
		static void Init(v8::Handle<v8::Object> exports);

	private:
		explicit VideoDemux();
		~VideoDemux();
		
		static void uv_DemuxTimer(uv_timer_t *req, int status);
		static void uv_SeekAsync(uv_work_t *req);
		static void uv_SeekAsyncAfter(uv_work_t *req, int status);
		static void uv_DemuxAsync(uv_work_t *req);
		static void uv_DemuxAsyncAfter(uv_work_t *req, int status);
		static void m_Error(DemuxBaton *btn, std::string msg);
		static void m_MetaData(DemuxBaton *btn, int width, int height, int64_t num_frames, double frame_rate, double duration, std::string format);
		static void m_Start(DemuxBaton *btn);
		static void m_End(DemuxBaton *btn);
		static void m_Seek(DemuxBaton *btn);
		static void m_Frame(DemuxBaton *btn, VideoFrame *frm);
		static int m_DecodePacket(DemuxBaton *btn, int *got_frame, int cached);
		static void m_DecodeFrame(DemuxBaton *btn);
		
		static v8::Handle<v8::Value> New(const v8::Arguments& args);
		static v8::Handle<v8::Value> LoadVideo(const v8::Arguments& args);
		static v8::Handle<v8::Value> StartDemuxing(const v8::Arguments& args);
		static v8::Handle<v8::Value> PauseDemuxing(const v8::Arguments& args);
		static v8::Handle<v8::Value> StopDemuxing(const v8::Arguments& args);
		static v8::Handle<v8::Value> VideoStopped(const v8::Arguments& args);
		static v8::Handle<v8::Value> SeekVideo(const v8::Arguments& args);
		static v8::Handle<v8::Value> On(const v8::Arguments& args);
		static v8::Persistent<v8::Function> constructor;
		
		void m_LoadVideo(std::string fn);
		void m_StartDemuxing();
		void m_PauseDemuxing();
		void m_StopDemuxing(v8::Persistent<v8::Function> callback);
		void m_VideoStopped();
		void m_SeekVideo(double timestamp, v8::Persistent<v8::Function> callback);
		int m_OpenCodecContext(int *stream_idx, AVFormatContext *fctx);
		void m_On(std::string type, v8::Persistent<v8::Function> callback);
		
		DemuxBaton *baton;
};

#endif // VIDEODEMUX_H