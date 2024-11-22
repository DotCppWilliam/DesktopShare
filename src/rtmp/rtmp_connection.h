#pragma once

#include "rtmp.h"
#include "tcp_connection.h"
#include "event_loop.h"
#include "rtmp_handshake.h"
#include "channel.h"
#include "rtmp_chunk.h"
#include "amf_codec.h"

enum ConnState
{
	CS_HANDSHAKE,
	CS_START_CONN,
	CS_START_CREATE_STREAM,
	CS_START_DELETE_STREAM,
	CS_START_PLAY,
	CS_START_PUBLISH
};

enum ConnMode
{
	CM_RTMP_SERVER,
	CM_PUSH
};

class RtmpConnection : public TcpConnection
{
public:
	RtmpConnection(EventLoop* event_loop, SOCKET sock, ConnMode conn_mode = CM_PUSH);
	~RtmpConnection();
public:
	bool Handshake();	// 和服务器进行RTMP握手
	void SetRtmpInfo(Rtmp* rtmp)
	{
		rtmp_info_ = rtmp; 
	}
private:
	bool OnRead(Buffer& buffer);			// tcp接收数据后执行的回调函数
	bool HandleChunk(Buffer& buffer);		// 解析发送来的rtmp数据,然后调用HandleMessage去处理
	bool HandleMessage(RtmpMsg& rtmp_msg);	// 根据rtmp msg,做相应处理
	bool HandleInvoke(RtmpMsg& rtmp_msg);
	bool HandleResult(RtmpMsg& rtmp_msg);
	bool HandleOnStatus(RtmpMsg& rtmp_msg);
	bool CreateStream();
	bool Publish();

	void SendRtmpChunks(uint32_t csid, RtmpMsg& rtmp_msg);
	bool SendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payload_size);

// rtmp推流之前的一些交互,互相设置一些参数、建立流等
	void SetChunkSize();					
	bool Connect();
private:
	Rtmp rtmp_;
	RtmpHandshake handshake_;	// 封装RTMP握手
	ConnState conn_state_;		// RTMP状态
	ConnMode conn_mode_;		// RTMP模式,推流还是拉流
	RtmpChunk rtmp_chunk_;		// 封装、解析RTMP chunk
	AMFDecoder amf_decoder_;
	AMFEncoder amf_encoder_;
	AMFObjects meta_data_;
	std::string status_;
	bool is_publishing_ = false;

	double trasaction_id_ = 1;	// 事务id,用于发送rtmp控制消息携带的
	uint32_t stream_id_ = 0;

	Rtmp* rtmp_info_ = nullptr;
};