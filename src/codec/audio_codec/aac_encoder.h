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
	 * ��ȡAAC�������Ķ�������,����: ��Ƶ��ʽ��Ϣ���������.
	 * ��Щ��Ϣ���ᱣ����extradata��
	 */
	int GetExtradataSize(uint8_t* buf, int max_buf_size);

	/**
	 * ���뺯��: ��ԭʼPCM��Ƶ���ݱ���ΪAAC��ʽ
	 * ����:
	 *		pcm: ԭʼpcm����
	 *		samples: ������
	 * ����ֵ:
	 *		nullptr: �˴�û�л�ȡ����ɹ��������
	 *		��Ϊnullptr: �ɹ���������Ƶ����
	 */
	AVPacketPtr Encode(const uint8_t* pcm, int samples);
private:
	AVCodecContext* codec_context_ = nullptr;
	AVSampleFormat sample_fmt_ = AV_SAMPLE_FMT_NONE;
	std::unique_ptr<Resampler> audio_resampler_;
	int64_t pts_ = 0;
};