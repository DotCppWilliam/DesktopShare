#include "monitor_info.h"

#pragma comment(lib, "d3d9.lib")

std::vector<Monitor> GetMonitors()
{
	std::vector<Monitor> monitors;

	HRESULT hr = S_OK;

	IDirect3D9Ex* d3d9ex = nullptr;
	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
	if (FAILED(hr))
		return monitors;

	// 获取显示适配器的数量
	int adapter_count = d3d9ex->GetAdapterCount();

	for (int i = 0; i < adapter_count; i++)
	{
		Monitor monitor;
		memset(&monitor, 0, sizeof(Monitor));

		LUID luid = { 0 , 0 };
		hr = d3d9ex->GetAdapterLUID(i, &luid);	// 获取当前适配器的LUID(本地唯一标识符)
		if (FAILED(hr))
			continue;

		monitor.low_part = (uint64_t)luid.LowPart;
		monitor.high_part = (uint64_t)luid.HighPart;

		// 获取监视器的句柄
		HMONITOR hMonitor = d3d9ex->GetAdapterMonitor(i);
		if (hMonitor)
		{
			MONITORINFO monitor_info;	// 存储监视器信息
			monitor_info.cbSize = sizeof(MONITORINFO);
			// 获取有关监视器的信息
			BOOL ret = GetMonitorInfoA(hMonitor, &monitor_info);
			if (ret)
			{
				// 存储监视器位置和尺寸信息
				monitor.left = monitor_info.rcMonitor.left;
				monitor.right = monitor_info.rcMonitor.right;
				monitor.top = monitor_info.rcMonitor.top;
				monitor.bottom = monitor_info.rcMonitor.bottom;
				monitors.push_back(monitor);
			}
		}
	}

	d3d9ex->Release();
	return monitors;
}