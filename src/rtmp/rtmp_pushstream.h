#pragma once
/*
	��Ϊ���������� ��Ƶ����Ƶ����
	(ע: ��Ҫ�и���������������,Ȼ��������ͨ��������ȥ��ȡ����)
*/

#include "rtmp.h"
#include "event_loop.h"
#include "rtmp_connection.h"
#include "socket.h"
#include "rtmp_handshake.h"
#include <thread>

/*
����Ҫʲô:
	��������
	rtmp��Header��body
		ý������
		ý����Ϣ
	����URL, ��ȡip���˿ںš�stream
*/

enum RtmpPushErr_t
{
	RTMP_PUSH_SUCCESS		= 0,
	RTMP_PUSH_INVALID_URL	= -1,
	RTMP_PUSH_CONN_FAILE	= -2,	// �������ӷ�����ʧ��
};

class RtmpPush : public Rtmp
{
public:
	RtmpPush(EventLoop* event_loop);
	~RtmpPush();
public:
	RtmpPushErr_t StartPush(std::string url, int timeout);

	void Close();
	int PushVideoFrame();
	int PushAudioFrame();
private:
	MediaInfo media_info_;		// �洢�й���Ƶ����Ƶ����Ϣ
	std::thread encode_audio_thread_;	// ������Ƶ�߳�
	std::thread encode_video_thread_;	// ������Ƶ�߳�
	RtmpConnection* rtmp_conn_ = nullptr;
	SocketBase* socket_ = nullptr;
};