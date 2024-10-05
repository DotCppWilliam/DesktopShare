#include "dxgi_screen_capture.h"
#include <cstdint>
#include <fstream>
#include <QDebug>
#include <dxgi1_2.h>
#include <d3d11.h>

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"D3D11.lib")


#define CHECK(expr) \
	((expr) != DXGI_ERROR_NOT_FOUND)
#define ASSERT_HR(hr) assert(SUCCEEDED(hr))

#pragma pack(push, 1)
struct BMPFileHeader
{
	uint16_t bfType; // 文件类型，应该是0x4D42 ('BM')
	uint32_t bfSize; // 文件大小
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits; // 图像数据偏移
};

struct BMPInfoHeader
{
	uint32_t biSize; // 信息头的大小
	int32_t  biWidth; // 图像宽度
	int32_t  biHeight; // 图像高度
	uint16_t biPlanes; // 平面数，应该是1
	uint16_t biBitCount; // 每像素位数，24位
	uint32_t biCompression; // 压缩类型，通常为0（无压缩）
	uint32_t biSizeImage; // 图像数据大小
	int32_t  biXPelsPerMeter; // 水平分辨率
	int32_t  biYPelsPerMeter; // 垂直分辨率
	uint32_t biClrUsed; // 使用的颜色索引数
	uint32_t biClrImportant; // 重要的颜色索引数
};
#pragma pack(pop)

#include <iostream>
void SaveBitmap(const char* filename, const uint8_t* imageData, uint32_t width_, uint32_t height_, uint32_t rowPitch)
{
	BMPFileHeader fileHeader = { 0 };
	BMPInfoHeader infoHeader = { 0 };

	fileHeader.bfType = 0x4D42; // 'BM'
	fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + rowPitch * height_;
	fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

	infoHeader.biSize = sizeof(BMPInfoHeader);
	infoHeader.biWidth = width_;
	infoHeader.biHeight = -static_cast<int32_t>(height_); // BMP 使用负值表示从底到顶存储
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 32; // RGBA
	infoHeader.biCompression = 0;
	infoHeader.biSizeImage = rowPitch * height_;

	std::ofstream file(filename, std::ios::binary | std::ios::out);
	if (!file.is_open())
	{
		std::cout << "打开文件失败" << std::endl;
		return;
	}
	file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
	file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
	file.write(reinterpret_cast<const char*>(imageData), rowPitch * height_);
	file.close();
}







/// //////////////////////////////////////////////////////////////// DXGIScreenCapture
DXGIScreenCapture::DXGIScreenCapture()
{
	HRESULT hr = S_OK;
	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, 0,
		nullptr, 0,
		D3D11_SDK_VERSION,
		&device_, nullptr, &context_);
	ASSERT_HR(hr);

	IDXGIFactory* dxgi_factory;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)&dxgi_factory);
	ASSERT_HR(hr);

	IDXGIAdapter* dxgi_adapter;
	IDXGIOutput* dxgi_output;

	hr = dxgi_factory->EnumAdapters(0, &dxgi_adapter);
	ASSERT_HR(hr);

	dxgi_adapter->EnumOutputs(0, &dxgi_output);
	ASSERT_HR(hr);

	IDXGIOutput1* dxgi_output1;
	hr = dxgi_output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgi_output1);
	ASSERT_HR(hr);

	hr = dxgi_output1->DuplicateOutput(device_, &dxgi_dup_);
	ASSERT_HR(hr);

	DXGI_OUTDUPL_DESC desc;
	dxgi_dup_->GetDesc(&desc);
	screen_width_ = desc.ModeDesc.Width;
	screen_height_ = desc.ModeDesc.Height;

	D3D11_TEXTURE2D_DESC texture_desc = { 0 };
	texture_desc.Width = screen_width_;
	texture_desc.Height = screen_height_;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texture_desc.BindFlags = 0;
	texture_desc.SampleDesc = { 1, 0 };
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	//texture_desc.CPUAccessFlags = 0;
	//texture_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;  

	hr = device_->CreateTexture2D(&texture_desc, nullptr, &capture_texture_);
	ASSERT_HR(hr);

	

	/*dxgi_output1->Release();
	dxgi_output->Release();
	dxgi_adapter->Release();
	device_->Release();*/
	qDebug() << "1";
}



DXGIScreenCapture::~DXGIScreenCapture()
{
}

bool DXGIScreenCapture::TryCaptureFrame()
{
	DXGI_OUTDUPL_FRAME_INFO info;
	IDXGIResource* resource;
	HRESULT hr = S_OK;

	hr = dxgi_dup_->AcquireNextFrame(0, &info, &resource);
	if (FAILED(hr))
	{
		switch (hr)
		{
		case DXGI_ERROR_WAIT_TIMEOUT:
		case DXGI_ERROR_INVALID_CALL:
		case DXGI_ERROR_ACCESS_LOST:
			return false;
		}
		return false;
	}
	/*if (info.AccumulatedFrames == 0 || info.LastPresentTime.QuadPart == 0)
		return false;*/

	ID3D11Texture2D* res_texture;
	hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&res_texture);
	ASSERT_HR(hr);

	//screen_paint_->WglUnLock();
	//context_->CopyResource(capture_texture_, res_texture);
	//screen_paint_->WglLock();	// 加锁

	res_texture->Release();
	resource->Release();
	dxgi_dup_->Release();
	return true;
}

ID3D11Device* DXGIScreenCapture::GetDevice()
{
	return device_;
}

ID3D11Texture2D* DXGIScreenCapture::GetTexture()
{
	return capture_texture_;
}

void DXGIScreenCapture::Init()
{
}

void DXGIScreenCapture::SetWglLockCallback(WGLCallback callback)
{
	wgl_lock_callback_ = callback;
}

void DXGIScreenCapture::SetWglUnLockCallback(WGLCallback callback)
{
	wgl_unlock_callback_ = callback;
}

void DXGIScreenCapture::SetScreenPaintPtr(OpenglScreenPaint* ptr)
{
	screen_paint_ = ptr;
}