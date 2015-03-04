#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <node.h>

class VideoFrame {
	public:
		VideoFrame();
		VideoFrame(uint8_t *b, uint32_t s, uint32_t f);
		~VideoFrame();
		
		uint8_t* getBuffer();
		uint32_t getBufferSize();
		int64_t getFrameIndex();
		void setBuffer(uint8_t *b);
		void setBufferSize(uint32_t s);
		void setFrameIndex(int64_t f);
		
	private:
		uint8_t *buf;
		uint32_t size;
		int64_t frame_idx;
};

#endif // VIDEOFRAME_H