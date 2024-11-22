#pragma once

#include "monitor_info.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <memory>
#include <QTimer>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>


int GetShaderSource(const char* path, std::string& shader_source);

class OpenglScreenPaint : public QOpenGLWidget, QOpenGLFunctions_3_3_Core
{
	friend class ScreenCapture;
	Q_OBJECT
public:
	OpenglScreenPaint(QWidget* parent = nullptr);
	~OpenglScreenPaint();
public:
// 重载函数
	void UpdateFrame(const uint8_t* frame_data, int width, int height);
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
public:
	uint32_t GetImgSize()
	{
		return img_size_;
	}
	bool GetScreenFrameTexture(std::vector<uint8_t>& data_buf);
private:
// 成员函数
	void InitD3D11();
	void InitShader();
	bool CaptureFrame();
public slots:
// 槽函数
	void RenderScreenFrameSlot();
private:
	QTimer* timer_ = nullptr;
	ID3D11DeviceContext* d3d11_context_ = nullptr;
	IDXGIOutputDuplication* output_dup_ = nullptr;
	ID3D11Texture2D* d3d11_texture_ = nullptr;		// 用于显示的屏幕帧纹理
	ID3D11Texture2D* double_buffer_[2] = { nullptr, nullptr };	// 双缓冲区
	std::atomic_int curr_buf_index_ = -1;						// 指向双缓冲区的下标
	std::atomic_flag front_buf_flag_ = ATOMIC_FLAG_INIT;
	std::atomic_flag back_buf_flag_ = ATOMIC_FLAG_INIT;

	HANDLE dxgi_device_ = nullptr;
	HANDLE dxgi_texture_ = nullptr;
	GLuint opengl_texture_ = 0;
	GLuint program_ = 0;
	GLuint vao_ = 0;

	uint32_t screen_width_ = 0;
	uint32_t screen_height_ = 0;
	uint32_t img_size_ = 0;
	std::vector<Monitor> monitors_;
	std::atomic_flag atomic_lock_ = ATOMIC_FLAG_INIT;
};
