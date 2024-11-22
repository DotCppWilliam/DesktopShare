#include "rtmp_connection.h"
#include "rtmp_handshake.h"
#include "rtmp_msg.h"
#include "amf_codec.h"
#include <QDebug>

RtmpConnection::RtmpConnection(EventLoop* event_loop, SOCKET sock, ConnMode conn_mode)
	: TcpConnection(event_loop, sock), handshake_(HANDSHAKE_S0S1S2),
	conn_mode_(conn_mode)
{
	this->SetReadCallback([this](TcpConnection* conn, Buffer& buf) {
		return this->OnRead(buf); });
}

RtmpConnection::~RtmpConnection()
{
}

/**
 * �ͷ���������RTMP����
 */
bool RtmpConnection::Handshake()
{
	uint32_t req_size = 1 + 1536;	// C0 + C1�Ĵ�С
	std::shared_ptr<char> req(new char[req_size], std::default_delete<char[]>());
	// �ͻ������ȷ���C0+C1���ְ�
	handshake_.BuildC0C1(req.get(), req_size);
	this->Send(req, req_size);

	this->Recv();	// �ȴ����շ���˷���S0+S1+S2���ְ�.�����ǿͻ��˷���C2�������
	return true;
}

/**
 * tcp�������ݺ�ִ�еĻص�����
 */
bool RtmpConnection::OnRead(Buffer& buffer)
{
	if (handshake_.IsCompleted())
	{
		// �������
		return HandleChunk(buffer);	// ����rtmp chunk��
	}
	
	// �յ�����˷�����S0+S1+S2���ְ���ȫ,��Ҫ������ȡ
	if (handshake_.GetHandshakeState() == HANDSHAKE_S0S1S2 
		&& buffer.ReadableBytes() < (1 + 1536 + 1536))
	{
		return false;
	}

	std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
	int res_size = handshake_.Parse(buffer, res.get(), 4096);	// ������ʣ�����ֽ׶�
	if (res_size < 0)
		return false;

	if (res_size > 0)
		this->Send(res, res_size);	// ����ʣ�����ְ�

	if (handshake_.IsCompleted())	// rtmp���ֽ������
	{
		if (buffer.ReadableBytes() > 0)
			return HandleChunk(buffer);	// ��ʼ�������ֺ��rtmp chunk���ݰ�
		if (conn_mode_ == CM_PUSH)
		{
			this->SetChunkSize();	// ���ֳɹ���, �ͻ������ȷ���set chunk size����
			this->Connect();		// ��connect('xx')ָ��
		}
	}

	return true;
}


/**
 * ������������rtmp����,Ȼ�����HandleMessageȥ����
 */
bool RtmpConnection::HandleChunk(Buffer& buffer)
{
	int ret = -1;
	do 
	{
		RtmpMsg rtmp_msg;
		// ����rtmp message
		ret = rtmp_chunk_.Parse(buffer, rtmp_msg);
		if (ret >= 0)
		{
			if (rtmp_msg.IsCompleted())	// ������ɵĻ�,����ʣ���rtmp����
			{
				HandleMessage(rtmp_msg);
			}

			if (ret == 0)
				break;
		}
		else if (ret < 0)
			return false;
	} while (buffer.ReadableBytes() > 0);

	return true;
}

/**
 * ����rtmp message,����Ӧ����
 */
bool RtmpConnection::HandleMessage(RtmpMsg& rtmp_msg)
{
	bool ret = true;
	switch (rtmp_msg.type_id_)
	{
	case RES_VIDEO:

		break;
	case RES_AUDIO:

		break;
	case RES_INVOKE:	// [����],����˽�������.����˷���_result����
		ret = HandleInvoke(rtmp_msg);
		break;
	case RES_NOTIFY:	
		// TODO:
		break;
	case RES_SET_CHUNK_SIZE:
		rtmp_chunk_.SetInChunkSize(ReadUint32BE((uint8_t*)rtmp_msg.payload_.get()));
		break;
	case RES_BANDWIDTH_SIZE:
		// TODO:
		break;
	case RES_ACK:
		// TODO:
		break;
	case RES_ACK_SIZE:
		// TODO:
		break;
	case RES_USER_EVENT:
		// TODO:
		break;
	default:
		break;
	}

	return ret;
}

/**
 * // [����],����˽�������.����˷���_result����
 */
bool RtmpConnection::HandleInvoke(RtmpMsg& rtmp_msg)
{
	bool ret = true;
	amf_decoder_.Reset();

	// �Ƚ�����method�ַ���
	int bytes_used = amf_decoder_.Decode((const char*)rtmp_msg.payload_.get(),
		rtmp_msg.length_, 1);
	if (bytes_used < 0)
		return false;

	std::string method = amf_decoder_.GetString();	// ��ȡmethod

	if (conn_mode_ == CM_PUSH)
	{
		// ��������ʼ�������е�AMFObjects
		bytes_used += amf_decoder_.Decode(rtmp_msg.payload_.get() + bytes_used,
			rtmp_msg.length_ - bytes_used);

	// ��Ϊ������,�����ֻ�ᷢ��_result��onStatus,������������Ľ������
		if (method == "_result")
			ret = HandleResult(rtmp_msg);	// ����˷������������connect('xx')���ӵ�����
		else if (method == "onStatus")
			ret = HandleOnStatus(rtmp_msg);	// ��������������׼������������
	}

	return ret;
}

/**
 * ����˷������������connect('xx')���ӵ�����
 *		�����˷���connect('xx'),��ʾ������������
 *		�������ᷢ��_result����,�����������
 */
bool RtmpConnection::HandleResult(RtmpMsg& rtmp_msg)
{
	bool ret = false;

// ��������߻����ڸշ����� connect('xx')����
	if (conn_state_ == CS_START_CONN)	
	{
		if (amf_decoder_.HasObject("code"))	// �жϷ���˷��ͻ�����_result�Ƿ�Я��code����
		{
		// ��ȡcode�������AMFObject����
			AMFObject amf_obj = amf_decoder_.GetAMFObject("code");
			// ������͵���NetConnection.Connect.Success,�������˽�������
			if (amf_obj.amf_str_ == "NetConnection.Connect.Success")
			{
				CreateStream();	// ��ô���������� createStream����,�������˴����µ���
				ret = true;
			}
		}
	}
// ����˽��������˷��͵�createStream
	else if (conn_state_ == CS_START_CREATE_STREAM)
	{
		if (amf_decoder_.GetDoubleNum() > 0)
		{
			stream_id_ = (uint32_t)amf_decoder_.GetDoubleNum();
			if (conn_mode_ == CM_PUSH)
				this->Publish();	// �����˷���publish('xx'),��ʾ��ʼ����������
		}
	}

	return ret;
}

/**
 * �������� �����˷���publish('xx')������,������onStatus
 */
bool RtmpConnection::HandleOnStatus(RtmpMsg& rtmp_msg)
{
	bool ret = true;
	if (conn_state_ == CS_START_PUBLISH || conn_state_ == CS_START_PLAY)
	{
		if (amf_decoder_.HasObject("code"))	// �ж��Ƿ���code����
		{
		// ��ȡcode���Ե�AMF����
			AMFObject amf_obj = amf_decoder_.GetAMFObject("code");
			status_ = amf_obj.amf_str_;
			if (conn_mode_ == CM_PUSH)
			{
				if (status_ == "NetStream.Publish.Start")
				{
					is_publishing_ = true;	// ����˽��տ�ʼ��������,���ñ�־λ
					// ���Է���@setDataFrame(), �������ò���,��Ϊsps��pps����Я������Ƶ������Ϣ
					// ������׼��������
				}
				else if (status_ == "NetStream.publish.Unauthorized"
					|| status_ == "NetStream.Publish.BadConnection"
					|| status_ == "NetStream.Publish.BadName")
				{
					ret = false;
				}
			}
			
		}
	}

	if (conn_state_ == CS_START_DELETE_STREAM)
	{
		if (amf_decoder_.HasObject("code"))
		{
			AMFObject amf_obj = amf_decoder_.GetAMFObject("code");
			if (amf_obj.amf_str_ != "NetStream.Unpublish.Success")
			{
				ret = false;
			}
		}
	}

	return ret;
}

/**
 * ����˽���connect�����,�����˷��� createStream,��ʾ�������˴����µ���
 */
bool RtmpConnection::CreateStream()
{
	AMFObjects amf_objects;
	amf_encoder_.Reset();

// ��װamf����
	amf_encoder_.EncodeString("createStream", 12);
	amf_encoder_.EncodeDoubleNumber(trasaction_id_++);
	amf_encoder_.EncodeObjects(amf_objects);

	conn_state_ = CS_START_CREATE_STREAM;	// ���õ�ǰ״̬Ϊ�������˴�����
	SendInvokeMessage(RTMP_CSID_INVOAKE_ID, amf_encoder_.Data(), amf_encoder_.Size());

	return true;
}

/**
 * �����˷���publish('xx'),��ʾ��ʼ����������
 */
bool RtmpConnection::Publish()
{
	AMFObjects amf_obj;
	amf_encoder_.Reset();
	amf_encoder_.EncodeString("publish", 7);
	amf_encoder_.EncodeDoubleNumber(trasaction_id_++);
	amf_encoder_.EncodeObjects(amf_obj);
	amf_encoder_.EncodeString(rtmp_info_->GetStream().c_str(), rtmp_info_->GetStream().size());

	conn_state_ = CS_START_PUBLISH;	// ����״̬Ϊ׼����ʼ����
	SendInvokeMessage(RTMP_CSID_INVOAKE_ID, amf_encoder_.Data(), amf_encoder_.Size());

	return true;
}


void RtmpConnection::SendRtmpChunks(uint32_t csid, RtmpMsg& rtmp_msg)
{
	uint32_t capacity = rtmp_msg.length_ + rtmp_msg.length_ / DEFAULT_CHUNK_SIZE * 5 + 1024;
	std::shared_ptr<char> buffer(new char[capacity], std::default_delete<char[]>());

	int size = rtmp_chunk_.CreateChunk(csid, rtmp_msg, buffer.get(), capacity);
	if (size > 0)
		this->Send(buffer, size);
}

bool RtmpConnection::SendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payload_size)
{
	if (this->is_close_)
		return false;

	RtmpMsg rtmp_msg;
	rtmp_msg.type_id_ = RES_INVOKE;
	rtmp_msg.timestamp_ = 0;
	rtmp_msg.stream_id_ = stream_id_;
	rtmp_msg.payload_ = payload;
	rtmp_msg.length_ = payload_size;
	SendRtmpChunks(csid, rtmp_msg);

	return true;
}


///////////////////////////////////// rtmp����֮ǰ��һЩ����,��������һЩ��������������
void RtmpConnection::SetChunkSize()
{
	rtmp_chunk_.SetOutChunkSize(DEFAULT_CHUNK_SIZE);
	std::shared_ptr<char> data(new char[4], std::default_delete<char[]>());
	WriteUint32BE((uint8_t*)data.get(), DEFAULT_CHUNK_SIZE);

	RtmpMsg rtmp_msg;
	rtmp_msg.type_id_ = RES_SET_CHUNK_SIZE;
	rtmp_msg.payload_ = data;
	rtmp_msg.length_ = 4;
	
	SendRtmpChunks(RTMP_CSID_CONTROL_ID, rtmp_msg);
}

/**
 * �����˷���connect('app����')����
 */
bool RtmpConnection::Connect()
{
	AMFObjects amf_objects;
	amf_encoder_.Reset();
	amf_encoder_.EncodeString("connect", 7);
	amf_encoder_.EncodeDoubleNumber(trasaction_id_++);
	amf_objects["app"] = AMFObject(rtmp_info_->GetApp());
	amf_objects["type"] = AMFObject(rtmp_info_->GetType());	// ����Ϊnonprivate,�ò���.

	amf_objects["swfUrl"] = AMFObject(rtmp_info_->GetSwfUrl());
	amf_objects["tcUrl"] = AMFObject(rtmp_info_->GetTcUrl());

	amf_encoder_.EncodeObjects(amf_objects);
	conn_state_ = CS_START_CONN;	// ����״̬Ϊ �������������������

	SendInvokeMessage(RTMP_CSID_INVOAKE_ID, amf_encoder_.Data(), amf_encoder_.Size());
	return true;
}
