#pragma once


enum AvFrameType
{
	VIDEO_FRAME_I = 0x01,
	VIDEO_FRAME_P = 0x02,
	VIDEO_FRAME_B = 0x03,
	AUDIO_FRAME   = 0x11
};