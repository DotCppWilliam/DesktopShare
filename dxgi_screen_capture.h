#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <QObject>
#include <dxgi1_2.h>
#include <functional>
#include "opengl_screen_paint.h"


using WGLCallback = std::function<void()>;
class DXGIScreenCapture : public QObject
{
	Q_OBJECT
public:
	DXGIScreenCapture();
	~DXGIScreenCapture();
public:
	bool TryCaptureFrame();
	ID3D11Device* GetDevice();
	ID3D11Texture2D* GetTexture();
public:
	void SetWglLockCallback(WGLCallback callback);
	void SetWglUnLockCallback(WGLCallback callback);
	void SetScreenPaintPtr(OpenglScreenPaint* ptr);
private:
	friend class OpenglScreenPaint;
private:
	void Init();
private:
	ID3D11Device* device_ = nullptr;
	IDXGIAdapter* adapter_ = nullptr;
	ID3D11DeviceContext* context_ = nullptr;
	IDXGIOutputDuplication* dxgi_dup_ = nullptr;
	ID3D11Texture2D* capture_texture_ = nullptr;

	uint32_t screen_width_;
	uint32_t screen_height_;

	WGLCallback wgl_lock_callback_;
	WGLCallback wgl_unlock_callback_;

	OpenglScreenPaint* screen_paint_ = nullptr;
};