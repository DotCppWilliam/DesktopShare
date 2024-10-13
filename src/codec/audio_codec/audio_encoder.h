#pragma once

#include <cstdint>

struct AudioConfig
{
	uint32_t channels_ = 0;
	uint32_t samplerate_ = 0;
	uint32_t bitrate_ = 0;
	int sample_fmt_ = 0;
};


class AudioEncoder
{
public:
	AudioEncoder() {}
	virtual ~AudioEncoder() {}
	AudioEncoder(const AudioEncoder&) = delete;
	AudioEncoder& operator=(const AudioEncoder&) = delete;
public:
	virtual bool Init(AudioConfig& in_cfg, AudioConfig& out_cfg) = 0;
	virtual void Destroy() = 0;
	virtual uint32_t GetFrames() = 0;
	virtual uint32_t GetChannel() = 0;
	virtual uint32_t GetSamplerate() = 0;
protected:
	AudioConfig audio_cfg_;
	bool init_ = false;
};