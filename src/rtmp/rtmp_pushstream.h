#pragma once
/*
	作为推流端推送 音频、视频数据
	(注: 需要有个服务器接收数据,然后拉流端通过服务器去获取数据)
*/

#include "rtmp.h"
#include "event_loop.h"
#include "rtmp_connection.h"
#include "socket.h"
#include "rtmp_handshake.h"
#include <thread>

/*
都需要什么:
	建立握手
	rtmp的Header和body
		媒体数据
		媒体信息
	解析URL, 获取ip、端口号、stream
*/

enum RtmpPushErr_t
{
	RTMP_PUSH_SUCCESS		= 0,
	RTMP_PUSH_INVALID_URL	= -1,
	RTMP_PUSH_CONN_FAILE	= -2,	// 推流连接服务器失败
};

class RtmpPush : public Rtmp
{
public:
	RtmpPush(EventLoop* event_loop);
	~RtmpPush();
public:
	RtmpPushErr_t StartPush(std::string url, int timeout);

	void Close();
	int PushVideoFrame();
	int PushAudioFrame();
private:
	MediaInfo media_info_;		// 存储有关音频和视频的信息
	std::thread encode_audio_thread_;	// 编码音频线程
	std::thread encode_video_thread_;	// 编码视频线程
	RtmpConnection* rtmp_conn_ = nullptr;
	SocketBase* socket_ = nullptr;
};