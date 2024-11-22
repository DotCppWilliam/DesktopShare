#pragma once

#include <string>
#include <memory>

#define DEFAULT_CHUNK_SIZE	4096	// 默认rtmp chunk大小

enum RtmpCodecType
{
	RTMP_CODEC_H264	= 7,
	RTMP_CODEC_AAC	= 10
};


// 存储有关音频和视频的信息
struct MediaInfo
{
// 有关视频的信息
	uint8_t		video_codec_id_ = RTMP_CODEC_H264;	
	uint8_t		video_framerate_ = 0;
	uint32_t	video_width_ = 0;
	uint32_t	video_height_ = 0;
	// size, 缓冲区数据
	std::pair<uint32_t, std::shared_ptr<uint8_t>> sps_;	// 序列参数集,包含完整的配置信息
	std::pair<uint32_t, std::shared_ptr<uint8_t>> pps_;	// 图像参数集,宽和高
	std::pair<uint32_t, std::shared_ptr<uint8_t>> sei_;	// 额外的辅助信息


// 有关音频的信息
	uint8_t  audio_codec_id_ = RTMP_CODEC_AAC;
	uint32_t audio_channel_ = 0;
	uint32_t audio_samplerate_ = 0;
	uint32_t audio_frame_len_ = 0;
	std::pair<uint32_t, std::shared_ptr<uint8_t>> audio_extradata_;
};



class Rtmp
{
public:
	Rtmp() = default;
	~Rtmp() = default;
public:
	void SetChunkSize(uint32_t chunk_size);
	void SetPeerBandwidth(uint32_t peer_bandwidth);
	void SetWinAckSize(uint32_t win_ack_size);
	virtual bool ParseUrl(std::string url);

	uint32_t GetChunkSize();
	uint32_t GetPeerBandwidth();
	uint32_t GetWinAckSize();

	std::string GetApp();
	std::string GetType();
	std::string GetFlashVersion();
	std::string GetSwfUrl();
	std::string GetTcUrl();
	std::string GetStream();
protected:
// "rtmp://192.168.1.1:1935/live/stream?swfUrl=http://example.com/my.swf&tcUrl=http://example.com/my.tcUrl&type=live&flashver=1.0";
// 其中type_ = live
// app = live/stream
// flash_ver = 1.0
// swf = http://example.com/my.swf
// tc = http://example.com/my.tcUrl
// 但是现在tcUrl和swfUrl是一样的,因为swfUrl太老了没有用途了
	std::string app_;
	std::string type_ = "nonprivate";	// 一般不会用到这个参数,除非自定义服务器
	std::string flash_ver_;
	std::string swf_url_;	// url中的参数, 指向flash播放器(swf文件)的url
	std::string tc_url_;	// rtmp连接的目标路径,
	std::string stream_;	// 

	std::string ip_;
	uint16_t port_ = 0;
	uint32_t win_ack_size_ = 5000000;
	uint32_t peer_bandwidth_ = 5000000;
	uint32_t chunk_size_ = DEFAULT_CHUNK_SIZE;
};