#pragma once

#include <vector>
#include <d3d9.h>

struct Monitor
{
	// LUID(本地唯一标识符,需要两个来存储)
	uint64_t low_part;
	uint64_t high_part;

	// 描述监视器的边缘坐标
	int left;
	int top;
	int right;
	int bottom;
};

std::vector<Monitor> GetMonitors();