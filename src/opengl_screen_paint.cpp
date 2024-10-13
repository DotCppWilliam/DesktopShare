#include "opengl_screen_paint.h"
//#include "dxgi_screen_capture.h"
#include <QDebug>
#include <QTimer>
#include <Windows.h>
#include "opengl/glext.h"
#include "opengl/wglext.h"
#include "opengl/glcorearb.h"
#include <QObject>
#include <fstream>
#include <QCoreApplication>

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"D3D11.lib")


////////////////////////////////////// 宏
#define VERTEX_PATH		"./vertex.glsl"
#define FRAGMENT_PATH	"./fragment.glsl"


#define HERROR(func) \
	do { \
		if (func != 0) \
			return -1; \
	} while(0)
#define ASSERT_HR(hr) assert(SUCCEEDED(hr))

#define GET_WGL_FUNC(wgl_var) \
	( (decltype(wgl_var)) (wglGetProcAddress(#wgl_var)) )	





////////////////////////////////////// 静态函数指针
// wgl扩展函数的函数指针
static PFNWGLDXOPENDEVICENVPROC wglDXOpenDeviceNV = nullptr;	
static PFNWGLDXCLOSEDEVICENVPROC wglDXCloseDeviceNV = nullptr;
static PFNWGLDXREGISTEROBJECTNVPROC wglDXRegisterObjectNV = nullptr;
static PFNWGLDXUNREGISTEROBJECTNVPROC wglDXUnregisterObjectNV = nullptr;
static PFNWGLDXLOCKOBJECTSNVPROC wglDXLockObjectsNV = nullptr;
static PFNWGLDXUNLOCKOBJECTSNVPROC wglDXUnlockObjectsNV = nullptr;









////////////////////////////////////// 普通函数
int GetShaderSource(const char* path, std::string & shader_source)
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs.is_open())
		return -1;

	std::string line;
	while (getline(ifs, line))
	{
		shader_source += line;
		if (!ifs.eof())
			shader_source[shader_source.size() - 1] = '\n';
	}
	const char* ptr = shader_source.c_str();
	shader_source += "\0";
	ifs.close();

	return 0;
}








///////////////////////////////////////////////////////////////// 构造、析构函数
OpenglScreenPaint::OpenglScreenPaint(QWidget* parent)
	: QOpenGLWidget(parent)
{
	timer_ = new QTimer(this);
	connect(timer_, &QTimer::timeout, this, &OpenglScreenPaint::RenderScreenFrameSlot);
	monitors_ = GetMonitors();
}

OpenglScreenPaint::~OpenglScreenPaint()
{
	if (timer_)
	{
		delete timer_;
		timer_ = nullptr;
	}
}


///////////////////////////////////////////////////////////////// opengl重载函数
void OpenglScreenPaint::UpdateFrame(const uint8_t* frame_data, int width, int height)
{
	
	
}

void OpenglScreenPaint::initializeGL()
{
	initializeOpenGLFunctions();
	InitD3D11();
	InitShader();
	
	timer_->start(1);
}

void OpenglScreenPaint::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program_);
	glBindVertexArray(vao_);
	glBindTexture(GL_TEXTURE_2D, opengl_texture_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(0);
	glBindVertexArray(0);
}

void OpenglScreenPaint::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}



///////////////////////////////////////////////////////////////// 私有成员函数
void OpenglScreenPaint::InitD3D11()
{
	wglDXOpenDeviceNV = GET_WGL_FUNC(wglDXOpenDeviceNV);
	wglDXCloseDeviceNV = GET_WGL_FUNC(wglDXCloseDeviceNV);
	wglDXRegisterObjectNV = GET_WGL_FUNC(wglDXRegisterObjectNV);
	wglDXUnregisterObjectNV = GET_WGL_FUNC(wglDXUnregisterObjectNV);
	wglDXLockObjectsNV = GET_WGL_FUNC(wglDXLockObjectsNV);
	wglDXUnlockObjectsNV = GET_WGL_FUNC(wglDXUnlockObjectsNV);


	HRESULT hr = S_OK;
	ID3D11Device* device;
	hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE,
		NULL, 0,
		NULL, 0,
		D3D11_SDK_VERSION,
		&device,
		NULL,
		&d3d11_context_);
	ASSERT_HR(hr);

	IDXGIFactory1* factory;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)&factory);
	ASSERT_HR(hr);

	IDXGIAdapter* adapter;
	IDXGIOutput* output;
	hr = factory->EnumAdapters(0, &adapter);
	ASSERT_HR(hr);

	adapter->EnumOutputs(0, &output);
	ASSERT_HR(hr);

	IDXGIOutput1* output1;
	hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
	ASSERT_HR(hr);

	hr = output1->DuplicateOutput(device, &output_dup_);
	ASSERT_HR(hr);

	DXGI_OUTDUPL_DESC desc;
	output_dup_->GetDesc(&desc);
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
	hr = device->CreateTexture2D(&texture_desc, nullptr, &d3d11_texture_);

	// 测试 ==========================
	texture_desc.Usage = D3D11_USAGE_STAGING;
	texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texture_desc.MiscFlags = 0;
	hr = device->CreateTexture2D(&texture_desc, nullptr, &rgba_texture_);
	ASSERT_HR(hr);

	dxgi_device_ = wglDXOpenDeviceNV(device);
	assert(dxgi_device_);
	glGenTextures(1, &opengl_texture_);
	dxgi_texture_ = wglDXRegisterObjectNV(dxgi_device_, 
		d3d11_texture_, 
		opengl_texture_,
		GL_TEXTURE_2D, 
		WGL_ACCESS_READ_ONLY_NV);
	assert(dxgi_texture_);

	BOOL ret = wglDXLockObjectsNV(dxgi_device_, 1, &dxgi_texture_);
	assert(ret);

	
	factory->Release();
	adapter->Release();
	output->Release();
	output1->Release();
	device->Release();
}

void DumpHex(std::string& str)
{
	std::string tmp;
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] == '\n')
		{
			tmp += "\\n";
			tmp += "\n";
		}
		else
		{
			tmp += str[i];
		}
	}

	for (int i = 0; i < tmp.size(); i++)
	{
		auto it = tmp.find("\n");
		if (it != std::string::npos)
		{
			std::string sss = tmp.substr(0, it);
			qDebug() << QString::fromStdString(sss);
			tmp = tmp.substr(it + 1, tmp.size());
		}
	}
	qDebug() << tmp[0];
	qDebug() << "=============================================================";
}

void OpenglScreenPaint::InitShader()
{
	const char* vertexShader =
		"#version 330 core                                   \n"
		"out vec2 vTexCoord;                                 \n"
		"void main()                                         \n"
		"{                                                   \n"
		"    float x = -1.0 + float((gl_VertexID & 1) << 2); \n"
		"    float y = -1.0 + float((gl_VertexID & 2) << 1); \n"
		"    vTexCoord = vec2(x + 1.0, 1.0 - y) * 0.5;       \n"
		"    gl_Position = vec4(x, y, 0, 1);                 \n"
		"}                                                   \n"
		;

	const char* fragmentShader =
		"#version 330 core                         \n"
		"in vec2 vTexCoord;                        \n"
		"uniform sampler2D uTexture;               \n"
		"layout (location = 0) out vec4 oColor;    \n"
		"void main()                               \n"
		"{                                         \n"
		"   oColor = texture(uTexture, vTexCoord); \n"
		"}                                         \n"
		;

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertexShader, NULL);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragmentShader, NULL);
	glCompileShader(fs);

	program_ = glCreateProgram();
	glAttachShader(program_, vs);
	glAttachShader(program_, fs);
	glLinkProgram(program_);
	glUseProgram(program_);

	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
}

bool OpenglScreenPaint::CaptureFrame()
{
	DXGI_OUTDUPL_FRAME_INFO info;
	IDXGIResource* resource;

	output_dup_->ReleaseFrame();	// 释放之前捕获的屏幕帧数据
	HRESULT hr = output_dup_->AcquireNextFrame(0, &info, &resource);
	if (SUCCEEDED(hr))
	{
		ID3D11Texture2D* resource_texture;
		hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&resource_texture);
		ASSERT_HR(hr);

		BOOL ok = wglDXUnlockObjectsNV(dxgi_device_, 1, &dxgi_texture_);
		assert(ok);

		d3d11_context_->CopyResource(d3d11_texture_, resource_texture);

		ok = wglDXLockObjectsNV(dxgi_device_, 1, &dxgi_texture_);
		assert(ok);

		resource_texture->Release();
		resource->Release();
		return true;
	}
	return false;
}

void OpenglScreenPaint::TextureMapToMemory()
{
	HRESULT hr = S_OK;
	d3d11_context_->CopyResource(rgba_texture_, d3d11_texture_);
	D3D11_MAPPED_SUBRESOURCE mapped_resource = { 0 };
	hr = d3d11_context_->Map(rgba_texture_, 0, D3D11_MAP_READ, 0, &mapped_resource);
	ASSERT_HR(hr);
}

void OpenglScreenPaint::DrawMouseInfo()
{
	
}





///////////////////////////////////////////////////////////////// 槽函数
void OpenglScreenPaint::RenderScreenFrameSlot()
{
	while (!CaptureFrame());
	this->update();
}





