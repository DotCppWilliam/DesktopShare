#pragma once

#include <cstdint>
#include <vector>

struct Mointor
{
	Mointor()
		: low_part(0), high_part(0),
		left(0), top(0), right(0), bottom(0) {}

	uint64_t low_part;
	uint64_t high_part;

	int left, top, right, bottom;
};

std::vector<Monitor> GetMonitors();