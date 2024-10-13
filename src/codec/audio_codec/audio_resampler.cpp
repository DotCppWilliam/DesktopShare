#include "audio_resampler.h"
#include "av_common.h"
#include <QDebug>

Resampler::Resampler()
{
}

Resampler::~Resampler()
{
	Destroy();
}

bool Resampler::Init(AudioConfig& in_cfg, AudioConfig& out_cfg)
{
	if (init_)
		return false;

	int64_t in_channels_layout = av_get_default_channel_layout(in_cfg.channels_);
	int64_t out_channels_layout = av_get_default_channel_layout(out_cfg.channels_);

	swr_context_ = swr_alloc();

	av_opt_set_int(swr_context_, "in_channel_layout", in_channels_layout, 0);
	av_opt_set_int(swr_context_, "in_sample_rate", in_cfg.samplerate_, 0);
	av_opt_set_sample_fmt(swr_context_, "in_sample_fmt", (AVSampleFormat)in_cfg.sample_fmt_, 0);

	av_opt_set_int(swr_context_, "out_channel_layout", out_channels_layout, 0);
	av_opt_set_int(swr_context_, "out_sample_rate", out_cfg.samplerate_, 0);
	av_opt_set_sample_fmt(swr_context_, "out_sample_fmt", (AVSampleFormat)out_cfg.sample_fmt_, 0);
	
	int ret = swr_init(swr_context_);
	if (ret < 0)
	{
		AV_ERR(ret, "swr_init failed");
		Destroy();
		return false;
	}

	in_audio_cfg_ = in_cfg;
	in_bits_per_sample_ = av_get_bytes_per_sample((AVSampleFormat)in_cfg.sample_fmt_);

	out_audio_cfg_ = out_cfg;
	out_bits_per_sample_ = av_get_bytes_per_sample((AVSampleFormat)out_cfg.sample_fmt_);

	return true;
}

void Resampler::Destroy()
{
	if (swr_context_ != nullptr)
	{
		if (swr_is_initialized(swr_context_))
			swr_close(swr_context_);

		swr_free(&swr_context_);
		swr_context_ = nullptr;
	}

	if (convert_buffer_)
	{
		av_free(convert_buffer_);
		convert_buffer_ = nullptr;
	}
}

int Resampler::Resample(AVFramePtr in_frame, AVFramePtr& out_frame)
{
	if (!init_)
		return -1;

	out_frame.reset(av_frame_alloc(), [](AVFrame* ptr) {
		av_frame_free(&ptr); 
	});

// 设置输出帧的属性
	out_frame->sample_rate = out_audio_cfg_.samplerate_;
	out_frame->format = out_audio_cfg_.sample_fmt_;
	out_frame->channels = out_audio_cfg_.channels_;
	// 根据输出采样率的变化重新计算采样数
	out_frame->nb_samples = (int)av_rescale_rnd(in_frame->nb_samples, out_frame->sample_rate,
		in_frame->sample_rate, AV_ROUND_UP);
	out_frame->pts = out_frame->pkt_dts = in_frame->pts;

	if (av_frame_get_buffer(out_frame.get(), 0) != 0)
		return -1;

	int len = swr_convert(swr_context_, (uint8_t**)&out_frame->data, out_frame->nb_samples,
		(const uint8_t**)in_frame->data, in_frame->nb_samples);
	if (len < 0)
	{
		out_frame = nullptr;
		AV_ERR(len, "swr_convert failed");
		return -1;
	}

	return len;
}
