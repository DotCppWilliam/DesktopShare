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




// RTMP��Ϣͷ�ṹ: ʱ��� + ���� + 
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

	uint8_t timestamp[3];	// ���
	uint8_t length[3];		// ���
	uint8_t type_id = 0;	// ��Ϣ����,��1��6��id������ΪЭ�������Ϣ
	uint8_t stream_id[4];	// С��

	uint8_t padding[5];		// �ֽڶ���,���
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

	uint32_t	timestamp_ = 0;			// ��Ϣ��ʱ���			4�ֽ�	���
	uint32_t	length_ = 0;			// ��Ϣ����Ч���ݴ�С		3�ֽ�	���
	uint32_t	stream_id_ = 0;			// ��Ϣ����ʶ			3�ֽ�	С��
	uint8_t		type_id_ = 0;			// ��Ϣ����				1�ֽ�	��1��6��id������ΪЭ�������Ϣ

	uint32_t	extend_timestamp_ = 0;	// ��չʱ���			4�ֽ�
	uint64_t	complete_timestamp_ = 0;	// ����ʱ��� + ��չʱ���
	uint8_t		codec_id_ = 0;
	uint8_t		csid_ = 0;		// �������ֿ������ĸ���
	uint32_t	index_ = 0;		// ��¼��ǰrtmp chunk��������ݴ�С
	std::shared_ptr<char> payload_ = nullptr;	// �洢rtmp chunk������
};