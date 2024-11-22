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


enum RtmpCsid
{
	RTMP_CSID_CONTROL_ID	= 2,
	RTMP_CSID_INVOAKE_ID	= 3,
	RTMP_CSID_AUDIO_ID,
	RTMP_CSID_VIDEO_ID,
	RTMP_CSID_DATA_ID
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
#pragma pack()


// ����RTMP ���������еĽ׶�
enum RtmpEstablishmentStage
{
	RES_SET_CHUNK_SIZE  = 0x01,
	RES_ABORT_MSG		= 0x02,
	RES_ACK				= 0x03,
	RES_USER_EVENT		= 0x04,
	RES_ACK_SIZE		= 0x05,
	RES_BANDWIDTH_SIZE	= 0x06,
	RES_AUDIO			= 0x08,
	RES_VIDEO			= 0x09,
	RES_FLEX_MSG		= 0x11,
	RES_NOTIFY			= 0x12,
	RES_INVOKE			= 0x14,
	RES_FLASH_VIDEO		= 0x16
};

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