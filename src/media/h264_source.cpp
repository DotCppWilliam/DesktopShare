#include "h264_source.h"

#include <chrono>

uint32_t GetVideoTimestamp()
{
	auto time_point = std::chrono::time_point_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now());

	return (uint32_t)((time_point.time_since_epoch().count() + 500) / 1000 * 90);
}