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

///////////////////////////////////////// 1. ����basic header
	uint8_t basic_header = buf_ptr[bytes_used];
	bytes_used++;
// ��������id(cs id)
	uint8_t csid = basic_header & 0x3F;	// ��ȡ��2λ,Ҳ���ǿ���id(csid)
	if (csid == 0) // basic header��2���ֽ�
	{
		if (buf_size < (bytes_used + 2))	// ��������С����
			return 0;
		csid += buf_ptr[bytes_used] + 64;	// ��ȡ��һ���ֽڵ�csid + 64

		bytes_used++;
	}
	else if (csid == 1)	// basic header��3���ֽ�
	{
		if (bytes_used < (bytes_used + 3))
			return 0;

		csid += buf_ptr[bytes_used] + 64;		// ��2���ֽ� + 64
		csid += buf_ptr[bytes_used + 1] * 256;	// ��3���ֽ� * 256

		bytes_used += 2;
	}
// ����������(��2λ)
	uint8_t fmt = (basic_header >> 6);
	if (fmt >= 4)	// 2������λ����ܱ�ʾ3,������������
		return -1;

	uint32_t msg_header_size = kMsgHeaderSize_[fmt];	// ��ȡ��Ϣͷ�Ĵ�С
	if (buf_size < (bytes_used + msg_header_size))	// ��������С����
		return 0;	


///////////////////////////////////////// 2. ����message header
	RtmpMsgHeader msg_header;
	memcpy(&msg_header, buf_ptr + bytes_used, msg_header_size);
	bytes_used += msg_header_size;

	auto& rtmp_msg = rtmp_msgs_[csid];
	chunk_stream_id_ = rtmp_msg.csid_ = csid;
// ����0������1��msg header����ʱ��������ȡ�type id
	if (fmt == RTMP_MSG_TYPE_0 || fmt == RTMP_MSG_TYPE_1)
	{
		uint32_t length = ReadUint24BE((uint8_t*)msg_header.length);
		if (rtmp_msg.length_ != length || !rtmp_msg.payload_)
		{
			rtmp_msg.length_ = length;
			rtmp_msg.payload_.reset(new char[rtmp_msg.length_], std::default_delete<char[]>());
		}

		rtmp_msg.index_ = 0;	// ��һ��chunk
		rtmp_msg.type_id_ = msg_header.type_id;
	}

// ����0���е�stream id
	if (fmt == RTMP_MSG_TYPE_0)
		rtmp_msg.stream_id_ = ReadUint24LE((uint8_t*)&rtmp_msg.stream_id_);

// ��ȡ��չʱ���
	uint32_t timestamp = ReadUint24BE((uint8_t*)msg_header.timestamp);
	uint32_t extend_timestamp = 0;
	// ���ʱ�������0xffffff,������չʱ���
	if (timestamp >= 0xFFFFFF || rtmp_msg.timestamp_ >= 0xFFFFFF)
	{
		if (buf_size < (bytes_used + 4))
			return 0;

		// ��ȡ��չʱ���
		extend_timestamp = ReadUint32BE((uint8_t*)buf_ptr + bytes_used);
		bytes_used += 4;
	}
	
	if (rtmp_msg.index_ == 0)	// ��ʾ�ǵ�һ��rtmp chunk
	{
		if (fmt == RTMP_MSG_TYPE_0)
		{
			rtmp_msg.complete_timestamp_ = 0;	// ����ʱ�������Ϊ0
			rtmp_msg.timestamp_ = timestamp;	// ����ʱ���
			rtmp_msg.extend_timestamp_ = extend_timestamp;	// ������չʱ���
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
	buf.Retrieve(bytes_used);	// ������rtmp��ͷ��,�ͷ���Щ����

	return bytes_used;	// ����rtmp ͷ���Ĵ�С
}

int RtmpChunk::ParseChunkBody(Buffer& buf)
{
	uint32_t bytes_used = 0;
	uint8_t* buf_ptr = (uint8_t*)buf.ReadBegin();
	uint32_t buf_size = buf.ReadableBytes();

	if (chunk_stream_id_ < 0)	// ����rtmp ͷ����
		return -1;

	auto& rtmp_msg = rtmp_msgs_[chunk_stream_id_];
	uint32_t chunk_size = rtmp_msg.length_ - rtmp_msg.index_;

	if (chunk_size > in_chunk_size_)
		chunk_size = in_chunk_size_;	// �������Ĭ�Ͽ��С,������ΪĬ�Ͽ��С

	// ��黺�������Ƿ����㹻����������ȡ����Ŀ��С
	if (buf_size < (bytes_used + chunk_size))
		return 0;

	// ����ȡchunk_size�ֽں��Ƿ�ᳬ����Ϣ���ܳ���
	if (rtmp_msg.index_ + chunk_size > rtmp_msg.length_)
		return -1;

	// �洢������
	memcpy(rtmp_msg.payload_.get() + rtmp_msg.index_, buf_ptr + bytes_used, chunk_size);
	bytes_used += chunk_size;
	rtmp_msg.index_ += chunk_size;	// ��¼���������ݴ�С

	// ����Ƿ�ǰchunk�Ѿ��������
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
				// ���þ���ʱ�����ֵ
				if (rtmp_msg.timestamp_ >= 0xFFFFFF)
					rtmp_msg.complete_timestamp_ += rtmp_msg.extend_timestamp_;
				else
					rtmp_msg.complete_timestamp_ += rtmp_msg.timestamp_;

				out_rtmp_msg = rtmp_msg;	// ���÷���ֵ,���chunk�������
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
	if (csid >= 64 + 255)	// basic headerΪ3�ֽ�
	{
		buf[size++] = (fmt << 6) | 1;				// ��1�ֽ�: fmt����Ϊ��2λ, ��6λ����Ϊ1
		buf[size++] = (csid - 64) & 0xFF;			// ��2�ֽ�: ��64,ȡ��8λ
		buf[size++] = ((csid - 64) >> 8) & 0xFF;	// ��3�ֽ�: ��64,ȡ��8λ
	}
	else if (csid >= 64)	// basic headerΪ2�ֽ�
	{
		buf[size++] = (fmt << 6) | 0;		// ��fmt����6λ,���ú�6λΪ0
		buf[size++] = (csid - 64) & 0xFF;
	}
	else
		buf[size++] = (fmt << 6) | csid;

	return size;
}

int RtmpChunk::CreateMessageHeader(uint8_t fmt, RtmpMsg& rtmp_msg, char* buf)
{
	int len = 0;
	if (fmt <= 2)	// ���� 0��1��2����Ϣͷ,����ʱ���
	{
		if (rtmp_msg.complete_timestamp_ < 0xffffff)
			WriteUint24BE((uint8_t*)buf, (uint32_t)rtmp_msg.complete_timestamp_);
		else
			WriteUint24BE((uint8_t*)buf, 0xffffff);

		len += 3;
	}

	if (fmt <= 1)	// ����0��1����type id
	{
		WriteUint24BE((uint8_t*)buf + len, rtmp_msg.length_);
		len += 3;
		buf[len++] = rtmp_msg.type_id_;
	}

	if (fmt == 0)	// ����0��stream id,������ʶ���ĸ�chunk
	{
		WriteUint32LE((uint8_t*)buf + len, rtmp_msg.stream_id_);
		len += 4;
	}
	return len;
}

int RtmpChunk::CreateChunk(uint32_t csid, RtmpMsg& rtmp_msg, char* buf, uint32_t buf_size)
{
	uint32_t buf_off = 0, payload_off = 0;

	// ��������Ļ�������С
	uint32_t capacity = rtmp_msg.length_ + rtmp_msg.length_ / out_chunk_size_ * 5;
	if (buf_size < capacity)
		return -1;

// ����rtmp������ͷ: basic header �� message header
	buf_off += CreateBasicHeader(0, csid, buf + buf_off);
	buf_off += CreateMessageHeader(0, rtmp_msg, buf + buf_off);
	if (rtmp_msg.complete_timestamp_ >= 0xffffff)	// ��Ҫ��չʱ���
	{
		WriteUint32BE((uint8_t*)buf + buf_off, (uint32_t)rtmp_msg.complete_timestamp_);
		buf_off += 4;
	}

	while (rtmp_msg.length_ > 0)
	{
		if (rtmp_msg.length_ > out_chunk_size_)	// ��Ҫ�ֶ��chunk���洢����
		{
			memcpy(buf + buf_off, rtmp_msg.payload_.get() + payload_off,
				out_chunk_size_);

			payload_off += out_chunk_size_;
			buf_off += out_chunk_size_;
			rtmp_msg.length_ -= out_chunk_size_;

			// Ȼ������������ ������3 ���洢
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
			rtmp_msg.length_ = 0;	// ���rtmp chunk�Ѿ���װ���
			break;
		}
	}


	return buf_off;
}