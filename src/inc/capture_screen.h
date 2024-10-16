#pragma once
/**
 * ������Ļ֡,ʹ��˫����������,�������Ⱦʹ�õ��ǲ�ͬ���������
 * ����˫���������,ʹ�û����˿��
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
	void StartCapture();	// ����һ���̲߳�����Ļ֡

private:
	void ThreadFunc();
private:
	ID3D11Texture2D* double_buffer_[2];	// ˫������
	int curr_texture_index_;			// ָ��˫�������洢���µ���Ļ֡����
	ID3D11DeviceContext* d3d11_context_ = nullptr;
	IDXGIOutputDuplication* output_dup_ = nullptr;
	uint32_t screen_width_ = 0;
	uint32_t screen_height_ = 0;
	std::thread thread_;
	std::atomic_flag atomic_lock;
};
