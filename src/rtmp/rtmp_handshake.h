#pragma once

#include "buffer.h"
#include <cstdint>


#define RTMP_VERSION		3
#define RTMP_HANDSHAKE_SIZE	1536	// 握手包的大小


/*
	C0、S0: | version | 
			   1字节

	C1、S1: | time (4字节) |		时间戳,客户端应该使用此字段标识所有流块时刻,可以为0或任意值
			| zero (4字节) |		必须为0
			| random bytes |	包含任意数据,必须足够随机,防止与其他握手端混淆
			| random bytes(cont) |
			    总共1536字节

	C2、S2: { time(4字节) }	必须包含对端发送的时间戳
			{ time2(4字节) } 必须包含时间戳,取值为接收端发送来的握手包时刻
			{ random echo } 必须包含对端发送来的随机数据.握手双方可以使用时间1和时间2 来估算网络连接延迟(不一定有用)
			{ random echo(cont) }
				总共1536字节
*/



class RtmpHandshake
{
	enum State
	{
		HANDSHAKE_C0C1,
		HANDSHAKE_S0S1S2,
		HANDSHAKE_C2,
		HANDSHAKE_COMPLETE
	};
public:
	RtmpHandshake(State state);
	~RtmpHandshake();
public:
	int Parse(Buffer& in_buf, char* out_buf, uint32_t out_buf_size);
	int BuildC0C1(char* buf, uint32_t buf_size);
	bool IsCompleted() const;
private:
	State handshake_state_;
};