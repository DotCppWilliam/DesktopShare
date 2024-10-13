#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"

}
#include "audio_encoder.h"
#include "audio_resampler.h"
#include "av_common.h"


class AACEncoder : public AudioEncoder
{
public:
	AACEncoder();
	~AACEncoder();
public:
	bool Init(AudioConfig& in_cfg, AudioConfig& out_cfg) override;
	void Destroy() override;
	uint32_t GetFrames() override;
	uint32_t GetChannel() override;
	uint32_t GetSamplerate() override;

	/**
	 * 获取AAC编码器的额外数据,比如: 音频格式信息、解码参数.
	 * 这些信息都会保存在extradata中
	 */
	int GetExtradataSize(uint8_t* buf, int max_buf_size);

	/**
	 * 编码函数: 将原始PCM音频数据编码为AAC格式
	 * 参数:
	 *		pcm: 原始pcm数据
	 *		samples: 采样数
	 * 返回值:
	 *		nullptr: 此次没有获取编码成功后的数据
	 *		不为nullptr: 成功编码后的音频数据
	 */
	AVPacketPtr Encode(const uint8_t* pcm, int samples);
private:
	AVCodecContext* codec_context_ = nullptr;
	AVSampleFormat sample_fmt_ = AV_SAMPLE_FMT_NONE;
	std::unique_ptr<Resampler> audio_resampler_;
	int64_t pts_ = 0;
};