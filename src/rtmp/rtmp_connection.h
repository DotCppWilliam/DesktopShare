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
	bool Handshake();	// �ͷ���������RTMP����
	void SetRtmpInfo(Rtmp* rtmp)
	{
		rtmp_info_ = rtmp; 
	}
private:
	bool OnRead(Buffer& buffer);			// tcp�������ݺ�ִ�еĻص�����
	bool HandleChunk(Buffer& buffer);		// ������������rtmp����,Ȼ�����HandleMessageȥ����
	bool HandleMessage(RtmpMsg& rtmp_msg);	// ����rtmp msg,����Ӧ����
	bool HandleInvoke(RtmpMsg& rtmp_msg);
	bool HandleResult(RtmpMsg& rtmp_msg);
	bool HandleOnStatus(RtmpMsg& rtmp_msg);
	bool CreateStream();
	bool Publish();

	void SendRtmpChunks(uint32_t csid, RtmpMsg& rtmp_msg);
	bool SendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payload_size);

// rtmp����֮ǰ��һЩ����,��������һЩ��������������
	void SetChunkSize();					
	bool Connect();
private:
	Rtmp rtmp_;
	RtmpHandshake handshake_;	// ��װRTMP����
	ConnState conn_state_;		// RTMP״̬
	ConnMode conn_mode_;		// RTMPģʽ,������������
	RtmpChunk rtmp_chunk_;		// ��װ������RTMP chunk
	AMFDecoder amf_decoder_;
	AMFEncoder amf_encoder_;
	AMFObjects meta_data_;
	std::string status_;
	bool is_publishing_ = false;

	double trasaction_id_ = 1;	// ����id,���ڷ���rtmp������ϢЯ����
	uint32_t stream_id_ = 0;

	Rtmp* rtmp_info_ = nullptr;
};