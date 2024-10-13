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
	case HANDSHAKE_C0C1:	// �ӽ�[�����]: ����������S0��S1��S2���ְ�
	{
	// �������˸÷���S0
	
		// ���С��1537(1 + 1536),��û��C0��C1������
		if (buf_size < RTMP_HANDSHAKE_SIZE)	
			return -1;

		if (buf[0] != RTMP_VERSION)	// �Է�RTMP�汾������3��֧��
			return -1;

		pos += 1537;
		res_size = 1 + 1536 + 1536;
		memset(out_buf, 0, 1537);	

	// ����S0���ְ��İ汾��
		out_buf[0] = RTMP_VERSION;	

	// ��������S1�����ֵ
		char* p = out_buf + 9;	// ָ��S1�����ֵ����
		for (int i = 0; i < 1528; i++)	
			*p++ = rd();	// �������ֵ

	// ����S2���ְ�,����������ԶԷ�������.���Խ��п���
		memcpy(p, buf + 1, 1536);
		handshake_state_ = HANDSHAKE_C2;	// ����״̬Ϊ�ȴ��ͻ��˷���C2

		break;
	}
	case HANDSHAKE_S0S1S2:	// �ӽ�[�ͻ���]: �ͻ��˷���C2���ְ�
	{
		if (buf_size < (1 + 1536 + 1536))	// ���ְ�û�н���ȫ
			return res_size;

		if (buf[0] != RTMP_VERSION)	// ����˵�RTMP�汾�Ų�����3��֧��
			return -1;

		pos += 1 + 1536 + 1536;
		res_size = 1536;
		memcpy(out_buf, buf + 1, 1536);	// ����C2���ְ�,���ݱ����ǶԷ����͵�����

		handshake_state_ = HANDSHAKE_COMPLETE;	// �ͻ�������������
		break;
	}
	case HANDSHAKE_C2:	// �ӽ�[�����]: ����ͻ��˷�������C2���ְ�
	{
		if (buf_size < 1536)
			return res_size;

		pos = 1536;
		handshake_state_ = HANDSHAKE_COMPLETE;	// ���������������
		break;
	}
	}

	in_buf.Retrieve(pos);
	return res_size;
}

int RtmpHandshake::BuildC0C1(char* buf, uint32_t buf_size)
{
	uint32_t size = 1 + 1536;	// C0��C1���ݰ��ܴ�С
	memset(buf, 0, size);

	buf[0] = RTMP_VERSION;	// ����C0���ְ��İ汾

	std::random_device rd;
	uint8_t* p = (uint8_t*)buf + 9;	// ָ��C1�����ֵλ��
	for (int i = 0; i < 1528; i++)
		*p++ = rd();

	return size;
}

bool RtmpHandshake::IsCompleted() const
{
	return handshake_state_ == HANDSHAKE_COMPLETE;
}
