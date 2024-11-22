#pragma once

/**
 * ʹ��libx264�����
 */

extern "C"
{
#include "libavcodec/avcodec.h"
}
#include "video_encoder.h"
#include "video_resampler.h"
#include "av_common.h"
#include <memory>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>

class H264Encoder : public VideoEncoder
{
public:
	H264Encoder();
	~H264Encoder();
public:
	void SetVideoCodec(VideoCodec codec);
	bool Init(VideoConfig& config) override;
	void Destroy() override;
	void ForceIDR() override;
	void SetBitrate(uint32_t bitrate_kbps) override;

	int Encode(const uint8_t* frame_data, VideoConfig& video_cfg,
		uint32_t img_size, std::vector<uint8_t>& out_frame);

	int Encode(ID3D11Texture2D* texture, VideoConfig& video_cfg, uint32_t imgs_size, 
		std::vector<uint8_t>& out_frame);

	int GetSequenceParams(uint8_t* out_buf, int out_buf_size);
private:
	bool IsKeyFrame(const uint8_t* data, uint32_t size);
	bool InitLibx264();
	AVPacketPtr Libx264Encode(const uint8_t* frame_data, VideoConfig& cfg,
		uint32_t frame_data_size, uint64_t pts = 0);
private:
	int64_t pts_ = 0;
	VideoConfig cfg_;
	VideoCodec codec_ = VCODEC_LIBX264;		// Ĭ�ϲ��������libx264,֧��Ӳ����nvenc��qsv
	AVCodecContext* codec_context_ = nullptr;
	std::unique_ptr<VideoResampler> video_resampler_ = nullptr;	// ��Ƶ֡������
	bool force_idr_ = false;
};