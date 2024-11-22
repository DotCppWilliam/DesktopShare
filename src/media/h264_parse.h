#pragma once


#include <cstdint>
#include <utility>

using NAL = std::pair<uint8_t*, uint8_t*>;	// nal的起始和末尾地址


NAL FindNal(const uint8_t* data, uint32_t size);