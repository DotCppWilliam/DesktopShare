#include "video_resampler.h"
#include <QDebug>

VideoResampler::VideoResampler()
{
}

VideoResampler::~VideoResampler()
{
	Destroy();
}

bool VideoResampler::Init(VideoConfig& in_cfg, VideoConfig& out_cfg)
{
	if (init_)
		return false;

	sws_context_ = sws_getContext(in_cfg.width_, in_cfg.height_, (AVPixelFormat)in_cfg.pixel_fmt_,
		out_cfg.width_, out_cfg.height_, (AVPixelFormat)out_cfg.pixel_fmt_, SWS_BICUBIC, 
		nullptr, nullptr, nullptr);

	if (sws_context_ == nullptr)
	{
		qDebug() << "sws_getContext failed";
		return false;
	}
	out_cfg_ = out_cfg;
	return true;
}

void VideoResampler::Destroy()
{
	if (sws_context_)
	{
		sws_freeContext(sws_context_);
		sws_context_ = nullptr;
	}
}

int VideoResampler::Resample(AVFramePtr in_frame, AVFramePtr out_frame)
{
	if (!init_)
		return -1;

	out_frame.reset(av_frame_alloc(), [](AVFrame* ptr) {
		av_frame_free(&ptr); });

	out_frame->width = out_cfg_.width_;
	out_frame->height = out_cfg_.height_;
	out_frame->format = out_cfg_.pixel_fmt_;

	out_frame->pts = in_frame->pts;
	out_frame->pkt_dts = in_frame->pkt_dts;

	if (av_frame_get_buffer(out_frame.get(), 32) != 0)
		return -1;
	int out_height = sws_scale(sws_context_, in_frame->data, in_frame->linesize, 0, in_frame->height,
		out_frame->data, out_frame->linesize);

	if (out_height < 0)
		return -1;

	return out_height;
}
