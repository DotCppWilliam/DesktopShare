#include "aac_source.h"

#include <chrono>

uint32_t GetAudioTimestamp(uint32_t samplerate)
{
	auto time_point = std::chrono::time_point_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now());

	auto round_ms = time_point.time_since_epoch().count() + 500;
	auto sample_count = round_ms / 1000 * samplerate;
	return (uint32_t)(sample_count / 1000);
}