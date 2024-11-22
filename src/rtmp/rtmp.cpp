#include "rtmp.h"
#include <regex>
#include <vector>
#include <sstream>
#include <istream>

void Rtmp::SetChunkSize(uint32_t chunk_size)
{
	chunk_size_ = chunk_size;
}

void Rtmp::SetPeerBandwidth(uint32_t peer_bandwidth)
{
	peer_bandwidth_ = peer_bandwidth;
}

void Rtmp::SetWinAckSize(uint32_t win_ack_size)
{
	win_ack_size_ = win_ack_size;
}


uint32_t Rtmp::GetChunkSize()
{
	return chunk_size_;
}

uint32_t Rtmp::GetPeerBandwidth()
{
	return peer_bandwidth_;
}

uint32_t Rtmp::GetWinAckSize()
{
	return win_ack_size_;
}


std::string Rtmp::GetApp()
{
	return app_;
}

std::string Rtmp::GetType()
{
	return type_;
}

std::string Rtmp::GetFlashVersion()
{
	return flash_ver_;
}

std::string Rtmp::GetSwfUrl()
{
	return swf_url_;
}

std::string Rtmp::GetTcUrl()
{
	return tc_url_;
}

std::string Rtmp::GetStream()
{
	return stream_;
}






bool IsValidIp(const std::string& ip)
{
	std::regex ip_regex(R"((\d{1,3}\.){3}\d{1,3})");
	return std::regex_match(ip, ip_regex);
}

bool IsValidPort(int port) 
{
	return (port > 0 && port <= 65535);
}


bool Rtmp::ParseUrl(std::string url)
{
	if (url.empty())
		return false;

	std::regex rtmp_regex(R"(rtmp://([^:/]+)(?::(\d+))?(/.+?)(\?.*)?$)");
	std::smatch matchs;

	if (!std::regex_match(url, matchs, rtmp_regex))
		return false;

	ip_ = matchs[1].str();
	port_ = matchs[2].length() > 0 ? std::stoi(matchs[2]) : 1935; // 默认端口是 1935
	std::string path = matchs[3].str();

	if (!IsValidIp(ip_) || !IsValidPort(port_))
		return false;

	// 解析应用名称
	size_t first_slash = path.find('/', 1);
	if (first_slash == std::string::npos) 
	{
		app_ = path.substr(1); // 去掉开头的 /
		if (app_.find("/") == std::string::npos)	// 没有stream
			return false;
		tc_url_ = "rtmp://" + ip_ + ":" + std::to_string(port_) + "/" + app_;
	}
	else 
	{
		app_ = path.substr(1, first_slash - 1); // 去掉开头的 / 并获取 app 部分
		stream_ = path.substr(first_slash + 1);
		if (stream_.empty())	// 没有stream
			return false;
		tc_url_ = "rtmp://" + ip_ + ":" + std::to_string(port_) + "/" + app_ + "/" + stream_;
	}
	swf_url_ = tc_url_;

	return true;
}