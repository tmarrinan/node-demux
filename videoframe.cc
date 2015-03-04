#include "videoframe.h"

VideoFrame::VideoFrame() {
	buf = NULL;
	size = 0;
	frame_idx = 0;
};
		
VideoFrame::VideoFrame(uint8_t *b, size_t s, uint32_t f) {
	buf = b;
	size = s;
	frame_idx = f;
};
		
VideoFrame::~VideoFrame() {
	if(buf) delete[] buf;
};
uint8_t* VideoFrame::getBuffer() {
	return buf;
}
size_t VideoFrame::getBufferSize() {
	return size;
}
int64_t VideoFrame::getFrameIndex() {
	return frame_idx;
}
void VideoFrame::setBuffer(uint8_t *b) {
	buf = b;
}
void VideoFrame::setBufferSize(size_t s) {
	size = s;
}
void VideoFrame::setFrameIndex(int64_t f) {
	frame_idx = f;
}
