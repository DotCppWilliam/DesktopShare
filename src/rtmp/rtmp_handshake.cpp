#include "rtmp_handshake.h"
#include <random>

RtmpHandshake::RtmpHandshake(State state)
	: handshake_state_(state)
{
}

RtmpHandshake::~RtmpHandshake()
{
}

int RtmpHandshake::Parse(Buffer& in_buf, char* out_buf, uint32_t out_buf_size)
{
	uint8_t* buf = (uint8_t*)in_buf.ReadBegin();
	uint32_t buf_size = in_buf.ReadableBytes();
	uint32_t pos = 0;
	uint32_t res_size = 0;
	std::random_device rd;


	switch (handshake_state_)
	{
	case HANDSHAKE_C0C1:	// 视角[服务端]: 服务器发送S0、S1、S2握手包
	{
	// 服务器端该发送S0
	
		// 如果小于1537(1 + 1536),则没有C0和C1两个包
		if (buf_size < RTMP_HANDSHAKE_SIZE)	
			return -1;

		if (buf[0] != RTMP_VERSION)	// 对方RTMP版本不等于3则不支持
			return -1;

		pos += 1537;
		res_size = 1 + 1536 + 1536;
		memset(out_buf, 0, 1537);	

	// 设置S0握手包的版本号
		out_buf[0] = RTMP_VERSION;	

	// 下面设置S1的随机值
		char* p = out_buf + 9;	// 指向S1的随机值区域
		for (int i = 0; i < 1528; i++)	
			*p++ = rd();	// 设置随机值

	// 设置S2握手包,必须包含来自对方的内容.所以进行拷贝
		memcpy(p, buf + 1, 1536);
		handshake_state_ = HANDSHAKE_C2;	// 设置状态为等待客户端发送C2

		break;
	}
	case HANDSHAKE_S0S1S2:	// 视角[客户端]: 客户端发送C2握手包
	{
		if (buf_size < (1 + 1536 + 1536))	// 握手包没有接收全
			return res_size;

		if (buf[0] != RTMP_VERSION)	// 服务端的RTMP版本号不等于3则不支持
			return -1;

		pos += 1 + 1536 + 1536;
		res_size = 1536;
		memcpy(out_buf, buf + 1, 1536);	// 设置C2握手包,数据必须是对方发送的内容

		handshake_state_ = HANDSHAKE_COMPLETE;	// 客户端这边握手完成
		break;
	}
	case HANDSHAKE_C2:	// 视角[服务端]: 处理客户端发送来的C2握手包
	{
		if (buf_size < 1536)
			return res_size;

		pos = 1536;
		handshake_state_ = HANDSHAKE_COMPLETE;	// 服务端这边握手完成
		break;
	}
	}

	in_buf.Retrieve(pos);
	return res_size;
}

int RtmpHandshake::BuildC0C1(char* buf, uint32_t buf_size)
{
	uint32_t size = 1 + 1536;	// C0、C1数据包总大小
	memset(buf, 0, size);

	buf[0] = RTMP_VERSION;	// 设置C0握手包的版本

	std::random_device rd;
	uint8_t* p = (uint8_t*)buf + 9;	// 指向C1的随机值位置
	for (int i = 0; i < 1528; i++)
		*p++ = rd();

	return size;
}

bool RtmpHandshake::IsCompleted() const
{
	return handshake_state_ == HANDSHAKE_COMPLETE;
}
