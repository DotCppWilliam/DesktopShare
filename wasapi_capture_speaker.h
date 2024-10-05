#pragma once
/*
	²¶»ñÑïÉùÆ÷
*/

#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <mmreg.h>
#include <cstdint>

#define REFTIMES_PER_SEC		((1000) * (10000))
#define REFTIMES_PERMILLISEC	10000

class WASAPICaptureSpeaker
{
public:
	WASAPICaptureSpeaker();
	~WASAPICaptureSpeaker();
private:
	void Init();
	void SetFormat();
	int CopyAudioData(BYTE* audio_data, uint32_t num_frames_available);
public:
	void Start();
private:
	CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	IID IID_IAudioClient = __uuidof(IAudioClient);
	IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

	IMMDeviceEnumerator* enumerator_ = nullptr;
	IMMDevice* device_ = nullptr;
	IAudioClient* audio_client_ = nullptr;
	IAudioCaptureClient* capture_client_ = nullptr;
	WAVEFORMATEX* wave_format_ = nullptr;
	WAVEFORMATEXTENSIBLE* wave_format_exten_ = nullptr;
	uint32_t pkt_length_ = 0;


	REFERENCE_TIME request_duration_ = REFTIMES_PER_SEC;
	REFERENCE_TIME actual_duration_ = 0;
	uint32_t buffer_frame_count_ = 0;
	uint32_t num_frames_available_ = 0;
};