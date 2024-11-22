#pragma once


#include <winsock2.h>
#include <cstdint>

void SetNonBlock(SOCKET sock);
void SetReuseAddr(SOCKET sock);

/*
	����Nagle�㷨.����㷨��������ݰ�������,�Ӷ��������Ч��
	�������,TCP��С�����ݰ�(ͨ��1460�ֽ�)�����ڷ��ͻ�������,ֱ�ӻ�����������նԶ�ȷ�ϲŻᷢ�ͳ�ȥ
*/
void SetNoDelay(SOCKET sock);	

/*
	����TCP�������,���������Ч�Ժͷ�ֹ������������ر�
	���ڿ���ʱ��(ͨ����Сʱ)����̽���ȷ�������Ƿ���Ч
*/
void SetKeepAlive(SOCKET sock);
void SetSendBufSize(SOCKET sock, size_t size);
void SetRecvBufSize(SOCKET sock, size_t size);