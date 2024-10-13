#pragma once

extern "C"
{
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
}
#include "audio_encoder.h"
#include <cstdint>
#include <memory>

class Resampler
{
	using AVFramePtr = std::shared_ptr<AVFrame>;
public:
	Resampler();
	~Resampler();
	Resampler(const Resampler&) = delete;
	Resampler& operator=(const Resampler&) = delete;
public:
	bool Init(AudioConfig& in_cfg, AudioConfig& out_cfg);

	void Destroy();

	int Resample(AVFramePtr in_frame, AVFramePtr& out_frame);
private:
	SwrContext* swr_context_ = nullptr;
	uint8_t** dst_buf_ = nullptr;

	int in_bits_per_sample_ = 0;
	AudioConfig in_audio_cfg_;

	int out_bits_per_sample_ = 0;
	AudioConfig out_audio_cfg_;

	int convert_buffer_size_ = 0;
	uint8_t* convert_buffer_ = nullptr;
	bool init_ = false;
};