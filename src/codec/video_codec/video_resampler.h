#pragma once

#include "av_common.h"
#include "video_encoder.h"
extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

class VideoResampler
{
public:
	VideoResampler();
	~VideoResampler();
	VideoResampler(const VideoResampler&) = delete;
	VideoResampler& operator=(const VideoResampler&) = delete;
public:
	bool Init(VideoConfig& in_cfg, VideoConfig& out_cfg);
	void Destroy();
	int Resample(AVFramePtr in_frame, AVFramePtr out_frame);
private:
	VideoConfig in_cfg_;
	VideoConfig out_cfg_;
	SwsContext* sws_context_ = nullptr;
	bool init_ = false;
};