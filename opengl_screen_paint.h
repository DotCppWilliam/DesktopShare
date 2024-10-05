#pragma once

#include "monitor_info.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <memory>
#include <QTimer>
#include <d3d11.h>
#include <dxgi1_2.h>


int GetShaderSource(const char* path, std::string& shader_source);

class OpenglScreenPaint : public QOpenGLWidget, QOpenGLFunctions_3_3_Core
{
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
private:
// 成员函数
	void InitD3D11();
	void InitShader();
	bool CaptureFrame();
	void TextureMapToMemory();
	void DrawMouseInfo();
public slots:
// 槽函数
	void RenderScreenFrameSlot();
private:
	QTimer* timer_ = nullptr;
	ID3D11DeviceContext* d3d11_context_ = nullptr;
	IDXGIOutputDuplication* output_dup_ = nullptr;
	ID3D11Texture2D* d3d11_texture_ = nullptr;
	ID3D11Texture2D* rgba_texture_ = nullptr;
	HANDLE dxgi_device_ = nullptr;
	HANDLE dxgi_texture_ = nullptr;
	GLuint opengl_texture_ = 0;
	GLuint program_ = 0;
	GLuint vao_ = 0;

	uint32_t screen_width_ = 0;
	uint32_t screen_height_ = 0;
	std::vector<Monitor> monitors_;
};
