#pragma once


#include <cstdint>

/**
 * 获取视频的时间戳,还有音频时间戳.
 * 用于客户端根据这俩时间戳进行同步
 */
uint32_t GetVideoTimestamp();