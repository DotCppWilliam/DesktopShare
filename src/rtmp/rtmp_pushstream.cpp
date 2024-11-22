#include "rtmp_pushstream.h"
#include "async_socket.h"

RtmpPush::RtmpPush(EventLoop* event_loop)
{
	socket_ = new AsyncSocket();
	socket_->Init();
	rtmp_conn_ = new RtmpConnection(event_loop, socket_->GetSocket());
	rtmp_conn_->SetRtmpInfo(this);
}

RtmpPush::~RtmpPush()
{
	if (socket_)
	{
		delete socket_;
		socket_ = nullptr;
	}

	if (rtmp_conn_)
	{
		delete rtmp_conn_;
		rtmp_conn_ = nullptr;
	}
}


/*
	1. 握手
	2. --> set chunk size
	3. --> connect('live')
	4. <-- win ack size
	5. <-- set peer bandwidth
	6. <-- set chunk size
	7. <-- _result('NetConnection.Connect.Success')
	8. --> releaseStream('stream')
	9. --> FCPublish('stream')
	10. --> createStream()
	11. <-- _result()
	12. --> publish('stream')
	13. <-- onStatus('NetStream.Publish.Start')
	14. --> @setDataFrame
	15. --> audio data
	16. --> video data
*/
RtmpPushErr_t RtmpPush::StartPush(std::string url, int timeout)
{
	if (!ParseUrl(url))
		return RTMP_PUSH_INVALID_URL;

	int ret = socket_->Connect(ip_, port_, 800, 5);	// 和服务器建立连接
	if (ret != 0)
		return RTMP_PUSH_CONN_FAILE;

	rtmp_conn_->Handshake();

	while (true);
	return RTMP_PUSH_SUCCESS;
}

void RtmpPush::Close()
{
}

int RtmpPush::PushVideoFrame()
{
	return 0;
}

int RtmpPush::PushAudioFrame()
{
	return 0;
}
