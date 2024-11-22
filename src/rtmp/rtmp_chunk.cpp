#include "rtmp_chunk.h"

RtmpChunk::RtmpChunk()
	: state_(PARSE_HEADER)
{
}

RtmpChunk::~RtmpChunk()
{
}




void RtmpChunk::Clear()
{
	rtmp_msgs_.clear();
}

int RtmpChunk::GetStreamId() const
{
	return stream_id_;
}

void RtmpChunk::SetOutChunkSize(uint32_t out_chunk_size)
{
	out_chunk_size_ = out_chunk_size;
}

void RtmpChunk::SetInChunkSize(uint32_t in_chunk_size)
{
	in_chunk_size_ = in_chunk_size;
}






int RtmpChunk::ParseChunkHeader(Buffer& buf)
{
	uint32_t bytes_used = 0;
	uint8_t* buf_ptr = (uint8_t*)buf.ReadBegin();
	uint32_t buf_size = buf.ReadableBytes();

///////////////////////////////////////// 1. 解析basic header
	uint8_t basic_header = buf_ptr[bytes_used];
	bytes_used++;
// 解析块流id(cs id)
	uint8_t csid = basic_header & 0x3F;	// 获取高2位,也就是块流id(csid)
	if (csid == 0) // basic header是2个字节
	{
		if (buf_size < (bytes_used + 2))	// 缓冲区大小不够
			return 0;
		csid += buf_ptr[bytes_used] + 64;	// 获取下一个字节的csid + 64

		bytes_used++;
	}
	else if (csid == 1)	// basic header是3个字节
	{
		if (bytes_used < (bytes_used + 3))
			return 0;

		csid += buf_ptr[bytes_used] + 64;		// 第2个字节 + 64
		csid += buf_ptr[bytes_used + 1] * 256;	// 第3个字节 * 256

		bytes_used += 2;
	}
// 解析块类型(高2位)
	uint8_t fmt = (basic_header >> 6);
	if (fmt >= 4)	// 2个比特位最多能表示3,如果大于则出错
		return -1;

	uint32_t msg_header_size = kMsgHeaderSize_[fmt];	// 获取消息头的大小
	if (buf_size < (bytes_used + msg_header_size))	// 缓冲区大小不够
		return 0;	


///////////////////////////////////////// 2. 解析message header
	RtmpMsgHeader msg_header;
	memcpy(&msg_header, buf_ptr + bytes_used, msg_header_size);
	bytes_used += msg_header_size;

	auto& rtmp_msg = rtmp_msgs_[csid];
	chunk_stream_id_ = rtmp_msg.csid_ = csid;
// 类型0和类型1的msg header都有时间戳、长度、type id
	if (fmt == RTMP_MSG_TYPE_0 || fmt == RTMP_MSG_TYPE_1)
	{
		uint32_t length = ReadUint24BE((uint8_t*)msg_header.length);
		if (rtmp_msg.length_ != length || !rtmp_msg.payload_)
		{
			rtmp_msg.length_ = length;
			rtmp_msg.payload_.reset(new char[rtmp_msg.length_], std::default_delete<char[]>());
		}

		rtmp_msg.index_ = 0;	// 第一个chunk
		rtmp_msg.type_id_ = msg_header.type_id;
	}

// 类型0独有的stream id
	if (fmt == RTMP_MSG_TYPE_0)
		rtmp_msg.stream_id_ = ReadUint24LE((uint8_t*)&rtmp_msg.stream_id_);

// 获取扩展时间戳
	uint32_t timestamp = ReadUint24BE((uint8_t*)msg_header.timestamp);
	uint32_t extend_timestamp = 0;
	// 如果时间戳大于0xffffff,则有扩展时间戳
	if (timestamp >= 0xFFFFFF || rtmp_msg.timestamp_ >= 0xFFFFFF)
	{
		if (buf_size < (bytes_used + 4))
			return 0;

		// 读取扩展时间戳
		extend_timestamp = ReadUint32BE((uint8_t*)buf_ptr + bytes_used);
		bytes_used += 4;
	}
	
	if (rtmp_msg.index_ == 0)	// 表示是第一个rtmp chunk
	{
		if (fmt == RTMP_MSG_TYPE_0)
		{
			rtmp_msg.complete_timestamp_ = 0;	// 绝对时间戳设置为0
			rtmp_msg.timestamp_ = timestamp;	// 设置时间戳
			rtmp_msg.extend_timestamp_ = extend_timestamp;	// 设置扩展时间戳
		}
		else
		{
			if (rtmp_msg.timestamp_ >= 0xFFFFFF)
				rtmp_msg.extend_timestamp_ += extend_timestamp;
			else
				rtmp_msg.timestamp_ += timestamp;
		}
	}

	state_ = PARSE_BODY;
	buf.Retrieve(bytes_used);	// 解析完rtmp的头部,释放这些数据

	return bytes_used;	// 返回rtmp 头部的大小
}

int RtmpChunk::ParseChunkBody(Buffer& buf)
{
	uint32_t bytes_used = 0;
	uint8_t* buf_ptr = (uint8_t*)buf.ReadBegin();
	uint32_t buf_size = buf.ReadableBytes();

	if (chunk_stream_id_ < 0)	// 解析rtmp 头出错
		return -1;

	auto& rtmp_msg = rtmp_msgs_[chunk_stream_id_];
	uint32_t chunk_size = rtmp_msg.length_ - rtmp_msg.index_;

	if (chunk_size > in_chunk_size_)
		chunk_size = in_chunk_size_;	// 如果大于默认块大小,则设置为默认块大小

	// 检查缓冲区中是否有足够的数据来读取所需的块大小
	if (buf_size < (bytes_used + chunk_size))
		return 0;

	// 检查读取chunk_size字节后是否会超过消息的总长度
	if (rtmp_msg.index_ + chunk_size > rtmp_msg.length_)
		return -1;

	// 存储块数据
	memcpy(rtmp_msg.payload_.get() + rtmp_msg.index_, buf_ptr + bytes_used, chunk_size);
	bytes_used += chunk_size;
	rtmp_msg.index_ += chunk_size;	// 记录这个块的数据大小

	// 检查是否当前chunk已经处理完毕
	if (rtmp_msg.index_ >= rtmp_msg.length_
		|| rtmp_msg.index_ % in_chunk_size_ == 0)
	{
		state_ = PARSE_HEADER;
	}

	buf.Retrieve(bytes_used);
	return bytes_used;
}

int RtmpChunk::Parse(Buffer& in_buf, RtmpMsg& out_rtmp_msg)
{
	if (!in_buf.ReadableBytes())
		return 0;

	switch (state_)
	{
	case PARSE_HEADER:
		return ParseChunkHeader(in_buf);
	case PARSE_BODY:
	{
		int ret = ParseChunkBody(in_buf);
		if (ret && chunk_stream_id_ >= 0)
		{
			auto& rtmp_msg = rtmp_msgs_[chunk_stream_id_];
			if (rtmp_msg.index_ == rtmp_msg.length_)
			{
				// 设置绝对时间戳的值
				if (rtmp_msg.timestamp_ >= 0xFFFFFF)
					rtmp_msg.complete_timestamp_ += rtmp_msg.extend_timestamp_;
				else
					rtmp_msg.complete_timestamp_ += rtmp_msg.timestamp_;

				out_rtmp_msg = rtmp_msg;	// 设置返回值,这个chunk解析完毕
				chunk_stream_id_ = -1;
				rtmp_msg.Clear();
			}
		}
		return ret;
	}
	}

	return 0;
}







int RtmpChunk::CreateBasicHeader(uint8_t fmt, uint32_t csid, char* buf)
{
	int size = 0;
	if (csid >= 64 + 255)	// basic header为3字节
	{
		buf[size++] = (fmt << 6) | 1;				// 第1字节: fmt设置为高2位, 低6位设置为1
		buf[size++] = (csid - 64) & 0xFF;			// 第2字节: 减64,取低8位
		buf[size++] = ((csid - 64) >> 8) & 0xFF;	// 第3字节: 减64,取高8位
	}
	else if (csid >= 64)	// basic header为2字节
	{
		buf[size++] = (fmt << 6) | 0;		// 将fmt左移6位,设置后6位为0
		buf[size++] = (csid - 64) & 0xFF;
	}
	else
		buf[size++] = (fmt << 6) | csid;

	return size;
}

int RtmpChunk::CreateMessageHeader(uint8_t fmt, RtmpMsg& rtmp_msg, char* buf)
{
	int len = 0;
	if (fmt <= 2)	// 类型 0、1、2有消息头,都有时间戳
	{
		if (rtmp_msg.complete_timestamp_ < 0xffffff)
			WriteUint24BE((uint8_t*)buf, (uint32_t)rtmp_msg.complete_timestamp_);
		else
			WriteUint24BE((uint8_t*)buf, 0xffffff);

		len += 3;
	}

	if (fmt <= 1)	// 类型0、1都有type id
	{
		WriteUint24BE((uint8_t*)buf + len, rtmp_msg.length_);
		len += 3;
		buf[len++] = rtmp_msg.type_id_;
	}

	if (fmt == 0)	// 类型0有stream id,用来标识是哪个chunk
	{
		WriteUint32LE((uint8_t*)buf + len, rtmp_msg.stream_id_);
		len += 4;
	}
	return len;
}

int RtmpChunk::CreateChunk(uint32_t csid, RtmpMsg& rtmp_msg, char* buf, uint32_t buf_size)
{
	uint32_t buf_off = 0, payload_off = 0;

	// 计算所需的缓冲区大小
	uint32_t capacity = rtmp_msg.length_ + rtmp_msg.length_ / out_chunk_size_ * 5;
	if (buf_size < capacity)
		return -1;

// 创建rtmp的两个头: basic header 和 message header
	buf_off += CreateBasicHeader(0, csid, buf + buf_off);
	buf_off += CreateMessageHeader(0, rtmp_msg, buf + buf_off);
	if (rtmp_msg.complete_timestamp_ >= 0xffffff)	// 需要扩展时间戳
	{
		WriteUint32BE((uint8_t*)buf + buf_off, (uint32_t)rtmp_msg.complete_timestamp_);
		buf_off += 4;
	}

	while (rtmp_msg.length_ > 0)
	{
		if (rtmp_msg.length_ > out_chunk_size_)	// 需要分多个chunk来存储数据
		{
			memcpy(buf + buf_off, rtmp_msg.payload_.get() + payload_off,
				out_chunk_size_);

			payload_off += out_chunk_size_;
			buf_off += out_chunk_size_;
			rtmp_msg.length_ -= out_chunk_size_;

			// 然后后面的数据用 块类型3 来存储
			buf_off += CreateBasicHeader(3, csid, buf + buf_off);
			if (rtmp_msg.complete_timestamp_ >= 0xfffff)
			{
				WriteUint32BE((uint8_t*)buf + buf_off, (uint32_t)rtmp_msg.complete_timestamp_);
				buf_off += 4;
			}
		}
		else
		{
			memcpy(buf + buf_off, rtmp_msg.payload_.get() + payload_off, rtmp_msg.length_);
			buf_off += rtmp_msg.length_;
			rtmp_msg.length_ = 0;	// 这个rtmp chunk已经封装完毕
			break;
		}
	}


	return buf_off;
}