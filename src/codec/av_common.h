#pragma once

extern "C"
{
#include "libavutil/error.h"
#include "libavcodec/avcodec.h"
}
#include <memory>

#define AV_ERR(err_num, str) \
	do { \
		char errbuf[256] = ""; \
		av_strerror(1, errbuf, sizeof(errbuf)); \
		qDebug() << str << errbuf; \
	} while(0)


using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;