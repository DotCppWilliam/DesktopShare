#pragma once

#include <cstdint>
#include <memory>


enum RtmpMsgType
{
	RTMP_MSG_TYPE_0,
	RTMP_MSG_TYPE_1,
	RTMP_MSG_TYPE_2,
	RTMP_MSG_TYPE_3,
};




// RTMP消息头结构: 时间戳 + 长度 + 
#pragma pack(1)
struct RtmpMsgHeader
{
	RtmpMsgHeader()
	{
		memset(timestamp, 0, sizeof(timestamp));
		memset(length, 0, sizeof(length));
		memset(stream_id, 0, sizeof(stream_id));
		memset(padding, 0, sizeof(padding));
	}

	uint8_t timestamp[3];	// 大端
	uint8_t length[3];		// 大端
	uint8_t type_id = 0;	// 消息类型,从1到6的id被保留为协议控制消息
	uint8_t stream_id[4];	// 小端

	uint8_t padding[5];		// 字节对齐,填充
};
#pragma pack(0)

struct RtmpMsg
{
	void Clear()
	{
		index_ = 0;
		timestamp_ = 0;
		extend_timestamp_ = 0;
		if (length_ > 0)
			payload_.reset(new char[length_], std::default_delete<char[]>());
	}

	bool IsCompleted() const
	{
		if (index_ == length_ && length_ && payload_)
			return true;

		return false;
	}

	uint32_t	timestamp_ = 0;			// 消息的时间戳			4字节	大端
	uint32_t	length_ = 0;			// 消息的有效数据大小		3字节	大端
	uint32_t	stream_id_ = 0;			// 消息流标识			3字节	小端
	uint8_t		type_id_ = 0;			// 消息类型				1字节	从1到6的id被保留为协议控制消息

	uint32_t	extend_timestamp_ = 0;	// 扩展时间戳			4字节
	uint64_t	complete_timestamp_ = 0;	// 保存时间戳 + 扩展时间戳
	uint8_t		codec_id_ = 0;
	uint8_t		csid_ = 0;		// 用来区分块属于哪个流
	uint32_t	index_ = 0;		// 记录当前rtmp chunk处理的数据大小
	std::shared_ptr<char> payload_ = nullptr;	// 存储rtmp chunk的数据
};