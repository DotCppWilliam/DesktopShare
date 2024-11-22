#include "h264_encoder.h"
extern "C"
{
#include "libavutil/opt.h"
}
#include "av_common.h"
#include <QDebug>



H264Encoder::H264Encoder()
{
}

H264Encoder::~H264Encoder()
{
	Destroy();
}

void H264Encoder::SetVideoCodec(VideoCodec codec)
{
	codec_ = codec;
}



bool H264Encoder::InitLibx264()
{
	AVCodec* codec = avcodec_find_encoder_by_name("libx264");
	if (codec == nullptr)
	{
		qDebug() << "find libx264 encoder failed";
		Destroy();
		return false;
	}

	codec_context_ = avcodec_alloc_context3(codec);
	if (codec_context_ == nullptr)
	{
		qDebug() << "avcodec_alloc_context3() failed";
		Destroy();
		return false;
	}

	codec_context_->width = cfg_.width_;
	codec_context_->height = cfg_.height_;
	codec_context_->time_base = { 1, (int)cfg_.framerate_ };
	codec_context_->framerate = { (int)cfg_.framerate_, 1 };
	codec_context_->gop_size = cfg_.gop_;
	codec_context_->max_b_frames = 0;
	codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;

// 设置ABR(平均比特率)
	codec_context_->bit_rate = cfg_.bitrate_;
	codec_context_->rc_min_rate = cfg_.bitrate_ / 2;
	codec_context_->rc_max_rate = cfg_.bitrate_ * 1.5;
	codec_context_->rc_buffer_size = cfg_.bitrate_;
	av_opt_set(codec_context_->priv_data, "nal-hrd", "abr", 0);

	codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
// 调整编码速度和质量的选项,ultrafast是最快但是质量最差的
	/*if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(codec_context_->priv_data, "preset", "faster", 0);*/

// 减少编码的延迟,会禁用掉某些可能引起延迟的特性,如B帧使用
	av_opt_set(codec_context_->priv_data, "tune", "zerolatency", 0);
// 是否强制生成IDR帧,设置为1代表生成
	av_opt_set_int(codec_context_->priv_data, "forced-idr", 1, 0);
// 设置编码等级, AVC-Intra是无损或近似无损编码格式.  -1表示不使用
	av_opt_set_int(codec_context_->priv_data, "avcintra-class", -1, 0);

	int ret = 0;
	if ((ret = avcodec_open2(codec_context_, codec, nullptr)) != 0)
	{
		AV_ERR(ret, "avcodec_open2 failed");
		Destroy();
		return false;
	}

	init_ = true;
	return true;
}



bool H264Encoder::Init(VideoConfig& config)
{
	if (init_)
		return false;

	cfg_ = config;
	switch (codec_)
	{
	case VCODEC_LIBX264:
		return InitLibx264();
	case VCODEC_NVECN:

	case VCODEC_QSV:

	default:
		return false;
	}

	return true;
}

void H264Encoder::Destroy()
{
	if (video_resampler_)
	{
		video_resampler_->Destroy();
		video_resampler_.reset();
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

void H264Encoder::ForceIDR()
{
	if (codec_context_)
		force_idr_ = true;
}

void H264Encoder::SetBitrate(uint32_t bitrate_kbps)
{
	if (codec_context_)
	{
		int64_t out_val = 0;
		av_opt_get_int(codec_context_->priv_data, "avcintra-class", 0, &out_val);
		if (out_val < 0)
		{
			codec_context_->bit_rate = bitrate_kbps * 1000;
			codec_context_->rc_min_rate = codec_context_->bit_rate;
			codec_context_->rc_max_rate = codec_context_->bit_rate;
			codec_context_->rc_buffer_size = codec_context_->bit_rate;
		}
	}
}


AVPacketPtr H264Encoder::Libx264Encode(const uint8_t* frame_data, VideoConfig& cfg, uint32_t frame_data_size, uint64_t pts)
{
	if (!init_)
		return nullptr;

	if (cfg.width_ != cfg_.width_ || cfg.height_ != cfg_.height_ || !video_resampler_)
	{
		video_resampler_.reset(new VideoResampler());
		if (!video_resampler_->Init(cfg, cfg_))
		{
			video_resampler_.reset();
			return nullptr;
		}
	}

	AVFramePtr in_frame(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
	in_frame->width = cfg_.width_;
	in_frame->height = cfg_.height_;
	in_frame->format = cfg_.pixel_fmt_;
	if (av_frame_get_buffer(in_frame.get(), 32) != 0)
		return nullptr;
	memcpy(in_frame->data[0], frame_data, frame_data_size);

	AVFramePtr yuv_frame = nullptr;
	if (video_resampler_->Resample(in_frame, yuv_frame) <= 0)
		return nullptr;

	if (pts >= 0)
		yuv_frame->pts = pts;
	else
		yuv_frame->pts = pts_++;

	yuv_frame->pict_type = AV_PICTURE_TYPE_NONE;
	if (force_idr_)
	{
		yuv_frame->pict_type = AV_PICTURE_TYPE_I;
		force_idr_ = false;
	}

	int ret = 0;
	if ((ret = avcodec_send_frame(codec_context_, yuv_frame.get())) < 0)
	{
		AV_ERR(ret, "avcodec_send_frame() failed");
		Destroy();
		return nullptr;
	}

	AVPacketPtr av_pkt(av_packet_alloc(), [](AVPacket* pkt) {
		av_packet_free(&pkt);
		});
	av_init_packet(av_pkt.get());

	ret = avcodec_receive_packet(codec_context_, av_pkt.get());
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		return nullptr;
	else if (ret < 0)
	{
		AV_ERR(ret, "avcodec_receive_packet() failed");
		Destroy();
		return nullptr;
	}

	return av_pkt;
}

int H264Encoder::Encode(const uint8_t* frame_data, VideoConfig& video_cfg, 
	uint32_t img_size, std::vector<uint8_t>& out_frame)
{
	if (!init_)
		return -1;
	int frame_size = 0;
	int max_buf_size = cfg_.width_ * cfg_.height_ * 4;	// 长 * 宽 * rgba(4通道)
	out_frame.clear();
	out_frame.resize(max_buf_size);

	switch (codec_)
	{
	case VCODEC_LIBX264:
	{
		AVPacketPtr pkt_ptr = Libx264Encode(frame_data, video_cfg, img_size);
		int extra_data_size = 0;
		uint8_t* extra_data = nullptr;

		if (pkt_ptr)
		{
			if (IsKeyFrame(pkt_ptr->data, pkt_ptr->size))
			{
				// 编码器使用了 AV_CODEC_FLAG_GLOBAL_HEADER, 这里需要添加sps、pps
				// sps: 视频序列集全局参数信息: 图像宽度、高度、帧率、颜色格式.这些存储到extra_data中
				// pps: 单个视频帧或多个视频帧解码参数: 帧内预测和帧间预测方式、量化参数
				// 这些在流媒体传输都是必不可少参数
				extra_data = codec_context_->extradata;
				extra_data_size = codec_context_->extradata_size;
				memcpy(out_frame.data(), extra_data, extra_data_size);
				frame_size += extra_data_size;
			}
			memcpy(out_frame.data() + frame_size, pkt_ptr->data, pkt_ptr->size);
			frame_size += pkt_ptr->size;
		}
		break;
	}
	case VCODEC_NVECN:
		break;
	case VCODEC_QSV:

		break;
	default: 
		return -1;
	}


	return frame_size;
}

int H264Encoder::Encode(ID3D11Texture2D* texture, VideoConfig& video_cfg, uint32_t imgs_size, std::vector<uint8_t>& out_frame)
{


	return 0;
}

int H264Encoder::GetSequenceParams(uint8_t* out_buf, int out_buf_size)
{
	if (!init_)
		return -1;

	int size = 0;
	switch (codec_)
	{
	case VCODEC_LIBX264:
	{
		size = codec_context_->extradata_size;
		memcpy(out_buf, codec_context_->extradata, codec_context_->extradata_size);
		break;
	}
	case VCODEC_NVECN:

		break;
	case VCODEC_QSV:
		break;
	default:
		return -1;
	}

	return size;
}

bool H264Encoder::IsKeyFrame(const uint8_t* data, uint32_t size)
{
	if (size > 4)
	{
		// 0x67:sps, 0x65:IDR, 0x6: SEI
		if (data[4] == 0x67 || data[4] == 0x65
			|| data[4] == 0x6 || data[4] == 0x27)
		{
			return true;
		}
	}
	return false;
}

