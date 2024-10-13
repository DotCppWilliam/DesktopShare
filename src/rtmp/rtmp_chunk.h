#pragma once
/**
 * ���ڽ���rtmp chunk��Ϣ
 */

#include "rtmp_msg.h"
#include "buffer.h"
#include <unordered_map>

class RtmpChunk
{
public:
	enum State
	{
		PARSE_HEADER,
		PARSE_BODY
	};
public:
	RtmpChunk();
	~RtmpChunk();
public:
	int Parse(Buffer& in_buf, RtmpMsg& out_rtmp_msg);
	int CreateChunk(uint32_t csid, RtmpMsg& rtmp_msg, char* buf, uint32_t buf_size);
	void Clear();
	int GetStreamId() const;
	void SetInChunkSize(uint32_t in_chunk_size);
	void SetOutChunkSize(uint32_t out_chunk_size);
private:
	int ParseChunkHeader(Buffer& buf);
	int ParseChunkBody(Buffer& buf);
	int CreateBasicHeader(uint8_t fmt, uint32_t csid, char* buf);
	int CreateMessageHeader(uint8_t fmt, RtmpMsg& rtmp_msg, char* buf);
private:
	const int kMsgHeaderSize_[4] = { 11, 7, 3, 0 };
	const int kDefaultStreamId_ = 1;

	State state_;
	int stream_id_ = kDefaultStreamId_;
	int chunk_stream_id_ = -1;		// ��¼chunk��steram id, ÿ��chunkֻ��һ��
	uint32_t in_chunk_size_ = 128;	// Ĭ��rtmp chunk��С
	uint32_t out_chunk_size_ = 128;
	std::unordered_map<int, RtmpMsg> rtmp_msgs_;	// ����id : rtmp msg

// ���basic header��, fmt(������)��Ӧ����Ϣͷ�Ĵ�С
// fmt = 0, msg header = 11�ֽ�
// fmt = 1, msg header = 7�ֽ�
// fmt = 2, msg header = 3�ֽ�
// fmt = 3, msg header = 0�ֽ�
};