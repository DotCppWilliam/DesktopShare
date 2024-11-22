#pragma once


#include <winsock2.h>
#include <cstdint>

void SetNonBlock(SOCKET sock);
void SetReuseAddr(SOCKET sock);

/*
	禁用Nagle算法.这个算法会减少数据包的数量,从而提高网络效率
	如果启用,TCP将小的数据包(通常1460字节)缓存在发送缓冲区中,直接缓冲区满或接收对端确认才会发送出去
*/
void SetNoDelay(SOCKET sock);	

/*
	开启TCP保活机制,检测连接有效性和防止空闲连接意外关闭
	会在空闲时间(通常几小时)发送探测包确认连接是否有效
*/
void SetKeepAlive(SOCKET sock);
void SetSendBufSize(SOCKET sock, size_t size);
void SetRecvBufSize(SOCKET sock, size_t size);