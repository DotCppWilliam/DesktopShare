#pragma once


#include <cstdint>
#include <utility>

using NAL = std::pair<uint8_t*, uint8_t*>;	// nal����ʼ��ĩβ��ַ


NAL FindNal(const uint8_t* data, uint32_t size);