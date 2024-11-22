#pragma once

#include <cstdint>

/**
 * 获取音频的时间戳,还有视频时间戳
 * 其中加入了音频采样率,为了和视频时间戳区分
 * 用于客户端根据这俩时间戳进行同步
 */
uint32_t GetAudioTimestamp(uint32_t samplerate);