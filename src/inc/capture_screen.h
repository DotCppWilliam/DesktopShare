#pragma once

#include <thread>
#include <QObject>
#include <d3d11.h>
#include <dxgi1_2.h>

class OpenglScreenPaint;

class ScreenCapture : public QObject
{
	Q_OBJECT
public:
	ScreenCapture(OpenglScreenPaint* screen_paint);
	~ScreenCapture();
public:
	void StartCapture();
	bool GetCaptureFrame(ID3D11Texture2D* texture);
signals:
	void CaptureSignal();
private:
	void ThreadFunc();
	bool AcquireFrame();
private:
	std::thread thread_;
	OpenglScreenPaint* screen_paint_ = nullptr;
	bool quit_ = false;
	bool first_ = true;
};