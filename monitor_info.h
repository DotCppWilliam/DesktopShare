#pragma once

#include <vector>
#include <d3d9.h>

struct Monitor
{
	// LUID(����Ψһ��ʶ��,��Ҫ�������洢)
	uint64_t low_part;
	uint64_t high_part;

	// �����������ı�Ե����
	int left;
	int top;
	int right;
	int bottom;
};

std::vector<Monitor> GetMonitors();