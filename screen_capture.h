#pragma once

#include <vector>

class ScreenCapture
{
public:
	ScreenCapture() = default;
	virtual ~ScreenCapture() {}
public:
	ScreenCapture(const ScreenCapture&) = delete;
	ScreenCapture& operator=(const ScreenCapture&) = delete;
public:
	virtual int Init(int display_index = 0) = 0;
	virtual int Destory() = 0;
	virtual int CaptureFrame(std::vector<uint8_t>& img, uint32_t& width, uint32_t& height) = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual bool IsStarted() const = 0;
};
