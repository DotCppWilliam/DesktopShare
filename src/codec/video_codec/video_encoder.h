#pragma once

#include <cstdint>

enum VideoCodec
{
	VCODEC_LIBX264,	// »Ì±‡¬Î
	VCODEC_NVECN,	// ”≤±‡¬Î
	VCODEC_QSV		// ”≤±‡¬Î
};

struct VideoConfig
{
	uint32_t width_ = 0;
	uint32_t height_ = 0;
	uint32_t bitrate_ = 0;
	uint32_t framerate_ = 0;
	uint32_t gop_ = 0;
	int pixel_fmt_ = 0;
};

class VideoEncoder
{
public:
	VideoEncoder() = default;
	virtual ~VideoEncoder() = default;

	VideoEncoder(const VideoEncoder&) = delete;
	VideoEncoder& operator=(const VideoEncoder&) = delete;
public:
	virtual bool Init(VideoConfig& config) = 0;
	virtual void Destroy() = 0;
	virtual void ForceIDR() = 0;
	virtual void SetBitrate(uint32_t bitrate_kbps) = 0;
protected:
	bool init_ = false;
};