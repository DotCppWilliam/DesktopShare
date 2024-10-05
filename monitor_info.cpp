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

	// ��ȡ��ʾ������������
	int adapter_count = d3d9ex->GetAdapterCount();

	for (int i = 0; i < adapter_count; i++)
	{
		Monitor monitor;
		memset(&monitor, 0, sizeof(Monitor));

		LUID luid = { 0 , 0 };
		hr = d3d9ex->GetAdapterLUID(i, &luid);	// ��ȡ��ǰ��������LUID(����Ψһ��ʶ��)
		if (FAILED(hr))
			continue;

		monitor.low_part = (uint64_t)luid.LowPart;
		monitor.high_part = (uint64_t)luid.HighPart;

		// ��ȡ�������ľ��
		HMONITOR hMonitor = d3d9ex->GetAdapterMonitor(i);
		if (hMonitor)
		{
			MONITORINFO monitor_info;	// �洢��������Ϣ
			monitor_info.cbSize = sizeof(MONITORINFO);
			// ��ȡ�йؼ���������Ϣ
			BOOL ret = GetMonitorInfoA(hMonitor, &monitor_info);
			if (ret)
			{
				// �洢������λ�úͳߴ���Ϣ
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