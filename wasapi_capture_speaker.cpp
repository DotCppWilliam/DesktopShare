#include "wasapi_capture_speaker.h"
#include <cassert>
#include <fstream>
#include <conio.h>
#include <QDebug>

#define ASSERT_HR(hr) assert(SUCCEEDED(hr))
#pragma warning(disable:4996)


FILE* file = nullptr;
DWORD file_size = 0;


struct waveheader
{
	//_byteswap_ulong
	DWORD riff = _byteswap_ulong(0x52494646);
	DWORD length;//little endian
	DWORD wave = _byteswap_ulong(0x57415645);
	DWORD fmt = _byteswap_ulong(0x666d7420);
	DWORD filter = (0x00000010);//16 18
	WORD FormatTag;//formattag
	WORD channel;//mono 1 stereo 2
	DWORD samplerate;
	DWORD bytespersec;//samplerate*bitsdepth*channel/8
	WORD samplesize;//bitsdepth*channel/8
	WORD bitsdepth;//32-bit 16-bit 8-bit
	//WORD exinfolength;
	//WORD wvalidbitsdepth;
	//DWORD dwChannelMask;
	//GUID SubFormat;//128b 16B
	DWORD data = _byteswap_ulong(0x64617461);
	DWORD datalength;
};

waveheader wave_header;

WASAPICaptureSpeaker::WASAPICaptureSpeaker()
{
	file = fopen("test.wav", "wb+");

	Init();
}

WASAPICaptureSpeaker::~WASAPICaptureSpeaker()
{
	fseek(file, 4, 0);
	file_size += -8;
	fwrite(&file_size, 4, 1, file);
	fseek(file, 36 + 4, 0);
	file_size += -36;
	fwrite(&file_size, 4, 1, file);
	fclose(file);

	if (enumerator_)
		enumerator_->Release();
	if (device_)
		device_->Release();
	if (audio_client_)
		audio_client_->Release();
	if (capture_client_)
		capture_client_->Release();
}

void WASAPICaptureSpeaker::Init()
{
	HRESULT hr = S_OK;

	// 用于枚举音频设备
	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, 
		nullptr, 
		CLSCTX_ALL, 
		IID_IMMDeviceEnumerator,
		(void**)&enumerator_);
	ASSERT_HR(hr);

	// 获取音频端点设备(扬声器)
	hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
	ASSERT_HR(hr);

	// 激活音频设备,用于音频管理
	hr = device_->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&audio_client_);
	ASSERT_HR(hr);

	// 获取音频设备的混合模式(即音频数据格式)
	hr = audio_client_->GetMixFormat(&wave_format_);
	ASSERT_HR(hr);
	wave_format_exten_ = (WAVEFORMATEXTENSIBLE*)wave_format_;

	hr = audio_client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		request_duration_,
		0,
		wave_format_,
		nullptr);
	ASSERT_HR(hr);

	// 获取音频缓冲区的大小(以帧为单位)
	hr = audio_client_->GetBufferSize(&buffer_frame_count_);
	ASSERT_HR(hr);

	// 准备开始捕获音频数据
	hr = audio_client_->GetService(IID_IAudioCaptureClient,
		(void**)&capture_client_);
	ASSERT_HR(hr);
	SetFormat();

	// 计算分配缓冲区的实际持续时间
	actual_duration_ = (double)REFTIMES_PER_SEC * buffer_frame_count_ 
		/ wave_format_->nSamplesPerSec;

}

void WASAPICaptureSpeaker::SetFormat()
{
	wave_header.FormatTag = WAVE_FORMAT_IEEE_FLOAT;
	wave_header.channel = wave_format_->nChannels;
	wave_header.samplerate = wave_format_->nSamplesPerSec;
	wave_header.bytespersec = wave_format_->nAvgBytesPerSec;
	wave_header.samplesize = wave_format_->nBlockAlign;
	wave_header.bitsdepth = wave_format_->wBitsPerSample;

	file_size = 36 + 8;
	fwrite(&wave_header, file_size, 1, file);
	fflush(file);
}








int WASAPICaptureSpeaker::CopyAudioData(BYTE* audio_data, uint32_t num_frames_available)
{
	WORD bytes_to_write;
	bytes_to_write = num_frames_available * wave_header.samplesize;
	WORD offset = 0;
	file_size += bytes_to_write;
	if (!audio_data)
	{
		BYTE zero[512] = { 0 };
		for (int i = 0; i < num_frames_available; i++)
			fwrite(zero, wave_header.samplesize, 1, file);
		qDebug() << QString::fromLocal8Bit("写入静音数据");
	}
	else
	{
		fwrite(audio_data, wave_header.samplesize, num_frames_available, file);
		qDebug() << QString::fromLocal8Bit("写入音频数据");
	}

	

	return 0;
}

void WASAPICaptureSpeaker::Start()
{
	HRESULT hr = S_OK;
	BYTE* audio_data = nullptr;
	DWORD flags;

	hr = audio_client_->Start();	
	ASSERT_HR(hr);

	
	int cnt = 999999999;
	while (cnt > 0)
	{
		hr = capture_client_->GetNextPacketSize(&pkt_length_);
		ASSERT_HR(hr);

		while (pkt_length_)
		{
			hr = capture_client_->GetBuffer(&audio_data,
				&num_frames_available_,
				&flags,
				nullptr, nullptr);
			ASSERT_HR(hr);

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)	// 无音频数据
				audio_data = nullptr;

			hr = CopyAudioData(audio_data, num_frames_available_);
			ASSERT_HR(hr);

			hr = capture_client_->ReleaseBuffer(num_frames_available_);
			ASSERT_HR(hr);

			hr = capture_client_->GetNextPacketSize(&pkt_length_);
			ASSERT_HR(hr);
		}
		cnt--;
	}
	audio_client_->Stop();
	CoTaskMemFree(wave_format_);
}
