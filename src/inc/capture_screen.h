#pragma once
/**
 * 捕获屏幕帧,使用双缓冲区机制,捕获和渲染使用的是不同的纹理对象
 * 这样双方交替进行,使得画面更丝滑
 */

#include <atomic>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <thread>

class CaptureFrame
{
public:
	CaptureFrame();
	~CaptureFrame();
public:
	void StartCapture();	// 启动一个线程捕获屏幕帧

private:
	void ThreadFunc();
private:
	ID3D11Texture2D* double_buffer_[2];	// 双缓冲区
	int curr_texture_index_;			// 指向双缓冲区存储最新的屏幕帧数据
	ID3D11DeviceContext* d3d11_context_ = nullptr;
	IDXGIOutputDuplication* output_dup_ = nullptr;
	uint32_t screen_width_ = 0;
	uint32_t screen_height_ = 0;
	std::thread thread_;
	std::atomic_flag atomic_lock;
};
