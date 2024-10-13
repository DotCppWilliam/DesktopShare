#include "aac_encoder.h"
#include "av_common.h"
#include <QDebug>

AACEncoder::AACEncoder()
{
}

AACEncoder::~AACEncoder()
{
}

bool AACEncoder::Init(AudioConfig& in_cfg, AudioConfig& out_cfg)
{
	if (init_)
		return false;
	int err = 0;

	audio_cfg_ = in_cfg;

// Ѱ��AAC������
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (codec == nullptr)
	{
		qDebug() << "find aac encoder failed";
		Destroy();
		return false;
	}
	
// ����codec������
	codec_context_ = avcodec_alloc_context3(codec);
	if (codec_context_ == nullptr)
	{
		qDebug() << "avcodec_alloc_context3 failed";
		Destroy();
		return false;
	}

// ���ò���
	codec_context_->sample_rate = audio_cfg_.samplerate_;
	codec_context_->sample_fmt = (AVSampleFormat)audio_cfg_.sample_fmt_;
	codec_context_->bit_rate = audio_cfg_.bitrate_;
	codec_context_->channels = audio_cfg_.channels_;
	codec_context_->channel_layout = av_get_default_channel_layout(audio_cfg_.channels_);

	if ((err = avcodec_open2(codec_context_, codec, nullptr)) != 0)
	{
		AV_ERR(err, "avcodec_open2 failed:");
		Destroy();
		return false;
	}

// ��ʼ����Ƶ������
	audio_resampler_.reset(new Resampler());
	if (!audio_resampler_->Init(audio_cfg_, out_cfg))
	{
		qDebug() << "audio resampler init failed";
		Destroy();
		return false;
	}

	init_ = true;
	return true;
}

void AACEncoder::Destroy()
{
	if (audio_resampler_)
	{
		audio_resampler_->Destroy();
		audio_resampler_.reset();
	}

	if (codec_context_)
	{
		avcodec_close(codec_context_);
		avcodec_free_context(&codec_context_);
		codec_context_ = nullptr;
	}
	pts_ = 0;
	init_ = false;
}

uint32_t AACEncoder::GetFrames()
{
	if (init_)
		return codec_context_->frame_size;
	return 0;
}

uint32_t AACEncoder::GetChannel()
{
	if (init_)
		return audio_cfg_.channels_;
	return 0;
}

uint32_t AACEncoder::GetSamplerate()
{
	if (init_)
		return audio_cfg_.samplerate_;
	return 0;
}

/**
 * ��ȡAAC�������Ķ�������,����: ��Ƶ��ʽ��Ϣ���������.
 * ��Щ��Ϣ���ᱣ����extradata��
 */
int AACEncoder::GetExtradataSize(uint8_t* buf, int max_buf_size)
{
	if (!init_)
		return -1;

	if (max_buf_size < codec_context_->extradata_size)
		return -1;

	memcpy(buf, codec_context_->extradata, codec_context_->extradata_size);

	return codec_context_->extradata_size;
}

/**
 * ���뺯��: ��ԭʼPCM��Ƶ���ݱ���ΪAAC��ʽ
 * ����:	
 *		pcm: ԭʼpcm����
 *		samples: ������
 * ����ֵ:
 *		nullptr: �˴�û�л�ȡ����ɹ��������
 *		��Ϊnullptr: �ɹ���������Ƶ����
 */
AVPacketPtr AACEncoder::Encode(const uint8_t* pcm, int samples)
{
	if (!init_)
		return nullptr;

	int ret = 0;
	// ����AVFrame�ṹ��,�洢Ҫ�������Ƶ֡����
	AVFramePtr in_frame(av_frame_alloc(), [](AVFrame* ptr) {
		av_frame_free(&ptr); });

	in_frame->sample_rate = audio_cfg_.samplerate_;
	in_frame->format = audio_cfg_.sample_fmt_;
	in_frame->channels = codec_context_->channels;
	in_frame->channel_layout = codec_context_->channel_layout;
	in_frame->nb_samples = samples;
	in_frame->pts = pts_;

	// ��pts_ת��Ϊ������ʱ�䵥λ��ʱ���
	in_frame->pts = av_rescale_q(pts_, { 1, codec_context_->sample_rate }, codec_context_->time_base);
	pts_ += in_frame->nb_samples;	// ÿ����һ������ʱ���

	// Ϊ��Աdata[]��linesize[]�����ڴ�
	if ((ret = av_frame_get_buffer(in_frame.get(), 0)) < 0)
	{
		AV_ERR(ret, "av_frame_get_buffer failed");
		return nullptr;
	}

	// ������Ƶÿ������ռ�ö����ֽ�
	int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)audio_cfg_.sample_fmt_);
	if (bytes_per_sample == 0)
		return nullptr;

	memcpy(in_frame->data[0], pcm, bytes_per_sample * in_frame->channels * samples);
	AVFramePtr fltp_frame = nullptr;
	// ����Ƶ�ز�������ĸ�ʽ,��ŵ�fltp_frame
	if (audio_resampler_->Resample(in_frame, fltp_frame) <= 0)
		return nullptr;

	// ��ת�������Ƶ֡���ݷ��͸�������,׼�����б���
	ret = avcodec_send_frame(codec_context_, fltp_frame.get());
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)	// ���������������������֡,����EAGIN��EOF
		return nullptr;
	
	// �洢������AAC����
	AVPacketPtr av_packet(av_packet_alloc(), [](AVPacket* ptr) {
		av_packet_free(&ptr); });
	// �ӱ�������ȡ�������ݰ�
	ret = avcodec_receive_packet(codec_context_, av_packet.get());
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		return nullptr;
	else if (ret < 0)
	{
		AV_ERR(ret, "avcodec_receive_packet failed");
		return nullptr;
	}

	return av_packet;
}
