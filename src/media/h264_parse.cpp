#include "h264_parse.h"
#include <cstring>

#define HAS_000001(start_code, pos) \
	((start_code[pos % 3] == 0) \
		&& (start_code[(pos + 1) % 3] == 0) \
		&& (start_code[(pos + 2) % 3] == 1))

#define HAS_000000(start_code, pos) \
	((start_code[pos % 3] == 0) \
		&& (start_code[(pos + 1) % 3] == 0) \
		&& (start_code[(pos + 2) % 3] == 0))

/**
 * 获取NAL的起始和末尾地址
 * 方法: 根据起始码00 00 00 01(或00 00 01),找到则表示NAL开头,然后找到下一个这样起始码,
 *		如果没有下一个起始码,那么则指向data的末尾地址
 */
NAL FindNal(const uint8_t* data, uint32_t size)
{
	NAL nal(nullptr, nullptr);

	if (size < 5)	// 00 00 00 01(起始码) / 00 00 01 + 帧类型(1字节).所以最起码5个字节
		return nal;
	
	// 指向末尾地址
	nal.second = const_cast<uint8_t*>(data) + (size - 1);

	uint32_t pos = 0;
	uint32_t start_code_size = 0;	// 起始码长度
	uint8_t start_code[3] = { 0 };	// 起始码

	memcpy(start_code, data, 3);
	size -= 3;
	data += 2;

	while (size--)
	{
		// 起始码是00 00 01
		if (HAS_000001(start_code, pos))
		{
			if (nal.first == nullptr)	// 如果还没有设置NAL起始地址
			{
			// 设置NAL起始地址和起始码的长度
				nal.first = const_cast<uint8_t*>(data) + 1;
				start_code_size = 3;
			}
			else if (start_code_size == 3)	// 已经设置过了NAL起始地址
			{
			// 设置NAL末尾地址
				nal.second = const_cast<uint8_t*>(data) - 3;
				break;	// 已经找到NAL单元了
			}
		}
		else if (HAS_000000(start_code, pos))
		{
			if (*(data + 1) == 0x01)	// 起始码为00 00 00 001
			{
				if (nal.first == nullptr)	// 还没有设置NAL起始地址
				{
					if (size >= 1)
						nal.first = const_cast<uint8_t*>(data) + 2;
				}
				else
					break;
			}
			else if (start_code_size == 4)
			{
				nal.second = const_cast<uint8_t*>(data) - 3;
				break;
			}
		}

		start_code[(pos++) % 3] = *(++data);
	}
	if (nal.first == nullptr)
		nal.second = nullptr;
	return nal;
}