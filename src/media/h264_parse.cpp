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
 * ��ȡNAL����ʼ��ĩβ��ַ
 * ����: ������ʼ��00 00 00 01(��00 00 01),�ҵ����ʾNAL��ͷ,Ȼ���ҵ���һ��������ʼ��,
 *		���û����һ����ʼ��,��ô��ָ��data��ĩβ��ַ
 */
NAL FindNal(const uint8_t* data, uint32_t size)
{
	NAL nal(nullptr, nullptr);

	if (size < 5)	// 00 00 00 01(��ʼ��) / 00 00 01 + ֡����(1�ֽ�).����������5���ֽ�
		return nal;
	
	// ָ��ĩβ��ַ
	nal.second = const_cast<uint8_t*>(data) + (size - 1);

	uint32_t pos = 0;
	uint32_t start_code_size = 0;	// ��ʼ�볤��
	uint8_t start_code[3] = { 0 };	// ��ʼ��

	memcpy(start_code, data, 3);
	size -= 3;
	data += 2;

	while (size--)
	{
		// ��ʼ����00 00 01
		if (HAS_000001(start_code, pos))
		{
			if (nal.first == nullptr)	// �����û������NAL��ʼ��ַ
			{
			// ����NAL��ʼ��ַ����ʼ��ĳ���
				nal.first = const_cast<uint8_t*>(data) + 1;
				start_code_size = 3;
			}
			else if (start_code_size == 3)	// �Ѿ����ù���NAL��ʼ��ַ
			{
			// ����NALĩβ��ַ
				nal.second = const_cast<uint8_t*>(data) - 3;
				break;	// �Ѿ��ҵ�NAL��Ԫ��
			}
		}
		else if (HAS_000000(start_code, pos))
		{
			if (*(data + 1) == 0x01)	// ��ʼ��Ϊ00 00 00 001
			{
				if (nal.first == nullptr)	// ��û������NAL��ʼ��ַ
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