#pragma once

#include "buffer.h"
#include <cstdint>


#define RTMP_VERSION		3
#define RTMP_HANDSHAKE_SIZE	1536	// ���ְ��Ĵ�С


/*
	C0��S0: | version | 
			   1�ֽ�

	C1��S1: | time (4�ֽ�) |		ʱ���,�ͻ���Ӧ��ʹ�ô��ֶα�ʶ��������ʱ��,����Ϊ0������ֵ
			| zero (4�ֽ�) |		����Ϊ0
			| random bytes |	������������,�����㹻���,��ֹ���������ֶ˻���
			| random bytes(cont) |
			    �ܹ�1536�ֽ�

	C2��S2: { time(4�ֽ�) }	��������Զ˷��͵�ʱ���
			{ time2(4�ֽ�) } �������ʱ���,ȡֵΪ���ն˷����������ְ�ʱ��
			{ random echo } ��������Զ˷��������������.����˫������ʹ��ʱ��1��ʱ��2 ���������������ӳ�(��һ������)
			{ random echo(cont) }
				�ܹ�1536�ֽ�
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