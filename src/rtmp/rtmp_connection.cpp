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
 * 和服务器进行RTMP握手
 */
bool RtmpConnection::Handshake()
{
	uint32_t req_size = 1 + 1536;	// C0 + C1的大小
	std::shared_ptr<char> req(new char[req_size], std::default_delete<char[]>());
	// 客户端首先发送C0+C1握手包
	handshake_.BuildC0C1(req.get(), req_size);
	this->Send(req, req_size);

	this->Recv();	// 等待接收服务端发送S0+S1+S2握手包.最后就是客户端发送C2握手完成
	return true;
}

/**
 * tcp接收数据后执行的回调函数
 */
bool RtmpConnection::OnRead(Buffer& buffer)
{
	if (handshake_.IsCompleted())
	{
		// 握手完成
		return HandleChunk(buffer);	// 处理rtmp chunk包
	}
	
	// 收到服务端发来的S0+S1+S2握手包不全,需要继续读取
	if (handshake_.GetHandshakeState() == HANDSHAKE_S0S1S2 
		&& buffer.ReadableBytes() < (1 + 1536 + 1536))
	{
		return false;
	}

	std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
	int res_size = handshake_.Parse(buffer, res.get(), 4096);	// 解析出剩余握手阶段
	if (res_size < 0)
		return false;

	if (res_size > 0)
		this->Send(res, res_size);	// 发送剩余握手包

	if (handshake_.IsCompleted())	// rtmp握手建立完毕
	{
		if (buffer.ReadableBytes() > 0)
			return HandleChunk(buffer);	// 则开始处理握手后的rtmp chunk数据包
		if (conn_mode_ == CM_PUSH)
		{
			this->SetChunkSize();	// 握手成功后, 客户端首先发送set chunk size命令
			this->Connect();		// 和connect('xx')指令
		}
	}

	return true;
}


/**
 * 解析发送来的rtmp数据,然后调用HandleMessage去处理
 */
bool RtmpConnection::HandleChunk(Buffer& buffer)
{
	int ret = -1;
	do 
	{
		RtmpMsg rtmp_msg;
		// 解析rtmp message
		ret = rtmp_chunk_.Parse(buffer, rtmp_msg);
		if (ret >= 0)
		{
			if (rtmp_msg.IsCompleted())	// 握手完成的话,处理剩余的rtmp交互
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
 * 根据rtmp message,做相应处理
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
	case RES_INVOKE:	// [推流],服务端接收连接.服务端发送_result命令
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
 * // [推流],服务端接收连接.服务端发送_result命令
 */
bool RtmpConnection::HandleInvoke(RtmpMsg& rtmp_msg)
{
	bool ret = true;
	amf_decoder_.Reset();

	// 先解析出method字符串
	int bytes_used = amf_decoder_.Decode((const char*)rtmp_msg.payload_.get(),
		rtmp_msg.length_, 1);
	if (bytes_used < 0)
		return false;

	std::string method = amf_decoder_.GetString();	// 获取method

	if (conn_mode_ == CM_PUSH)
	{
		// 接下来开始解析所有的AMFObjects
		bytes_used += amf_decoder_.Decode(rtmp_msg.payload_.get() + bytes_used,
			rtmp_msg.length_ - bytes_used);

	// 作为推流端,服务端只会发送_result和onStatus,处理推流请求的结果命令
		if (method == "_result")
			ret = HandleResult(rtmp_msg);	// 服务端发送针对推流端connect('xx')连接的请求
		else if (method == "onStatus")
			ret = HandleOnStatus(rtmp_msg);	// 服务端针对推流端准备推流的请求
	}

	return ret;
}

/**
 * 服务端发送针对推流端connect('xx')连接的请求
 *		推流端发送connect('xx'),表示发起连接请求
 *		服务端则会发送_result命令,代表接收请求
 */
bool RtmpConnection::HandleResult(RtmpMsg& rtmp_msg)
{
	bool ret = false;

// 推流端这边还属于刚发送完 connect('xx')命令
	if (conn_state_ == CS_START_CONN)	
	{
		if (amf_decoder_.HasObject("code"))	// 判断服务端发送回来的_result是否携带code属性
		{
		// 获取code属性这个AMFObject对象
			AMFObject amf_obj = amf_decoder_.GetAMFObject("code");
			// 如果发送的是NetConnection.Connect.Success,代表服务端接收连接
			if (amf_obj.amf_str_ == "NetConnection.Connect.Success")
			{
				CreateStream();	// 那么推流端则发送 createStream命令,请求服务端创建新的流
				ret = true;
			}
		}
	}
// 服务端接收推流端发送的createStream
	else if (conn_state_ == CS_START_CREATE_STREAM)
	{
		if (amf_decoder_.GetDoubleNum() > 0)
		{
			stream_id_ = (uint32_t)amf_decoder_.GetDoubleNum();
			if (conn_mode_ == CM_PUSH)
				this->Publish();	// 推流端发送publish('xx'),表示开始向服务端推流
		}
	}

	return ret;
}

/**
 * 服务端针对 推流端发送publish('xx')的请求,而发送onStatus
 */
bool RtmpConnection::HandleOnStatus(RtmpMsg& rtmp_msg)
{
	bool ret = true;
	if (conn_state_ == CS_START_PUBLISH || conn_state_ == CS_START_PLAY)
	{
		if (amf_decoder_.HasObject("code"))	// 判断是否有code属性
		{
		// 获取code属性的AMF对象
			AMFObject amf_obj = amf_decoder_.GetAMFObject("code");
			status_ = amf_obj.amf_str_;
			if (conn_mode_ == CM_PUSH)
			{
				if (status_ == "NetStream.Publish.Start")
				{
					is_publishing_ = true;	// 服务端接收开始推流请求,设置标志位
					// 可以发送@setDataFrame(), 但是作用不大,因为sps、pps都会携带音视频编码信息
					// 接下来准备发送流
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
 * 服务端接收connect请求后,推流端发送 createStream,表示请求服务端创建新的流
 */
bool RtmpConnection::CreateStream()
{
	AMFObjects amf_objects;
	amf_encoder_.Reset();

// 封装amf对象
	amf_encoder_.EncodeString("createStream", 12);
	amf_encoder_.EncodeDoubleNumber(trasaction_id_++);
	amf_encoder_.EncodeObjects(amf_objects);

	conn_state_ = CS_START_CREATE_STREAM;	// 设置当前状态为请求服务端创建流
	SendInvokeMessage(RTMP_CSID_INVOAKE_ID, amf_encoder_.Data(), amf_encoder_.Size());

	return true;
}

/**
 * 推流端发送publish('xx'),表示开始向服务端推流
 */
bool RtmpConnection::Publish()
{
	AMFObjects amf_obj;
	amf_encoder_.Reset();
	amf_encoder_.EncodeString("publish", 7);
	amf_encoder_.EncodeDoubleNumber(trasaction_id_++);
	amf_encoder_.EncodeObjects(amf_obj);
	amf_encoder_.EncodeString(rtmp_info_->GetStream().c_str(), rtmp_info_->GetStream().size());

	conn_state_ = CS_START_PUBLISH;	// 设置状态为准备开始推流
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


///////////////////////////////////// rtmp推流之前的一些交互,互相设置一些参数、建立流等
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
 * 向服务端发起connect('app名字')请求
 */
bool RtmpConnection::Connect()
{
	AMFObjects amf_objects;
	amf_encoder_.Reset();
	amf_encoder_.EncodeString("connect", 7);
	amf_encoder_.EncodeDoubleNumber(trasaction_id_++);
	amf_objects["app"] = AMFObject(rtmp_info_->GetApp());
	amf_objects["type"] = AMFObject(rtmp_info_->GetType());	// 设置为nonprivate,用不上.

	amf_objects["swfUrl"] = AMFObject(rtmp_info_->GetSwfUrl());
	amf_objects["tcUrl"] = AMFObject(rtmp_info_->GetTcUrl());

	amf_encoder_.EncodeObjects(amf_objects);
	conn_state_ = CS_START_CONN;	// 设置状态为 向服务器发起连接请求

	SendInvokeMessage(RTMP_CSID_INVOAKE_ID, amf_encoder_.Data(), amf_encoder_.Size());
	return true;
}
