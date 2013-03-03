#include <stdio.h>
#include <map>
#include <vector>
#include <string>
#include "Platinum.h"
#include "ProxyDevice.h"



typedef std::pair<std::string, std::string> DeviceInfo; // name, uuid
typedef std::vector<DeviceInfo> DisplayList;

static void DisplayDevices(const DeviceMap& device_map, DisplayList& list)
{
	printf("0. Refresh Menu\n");
	DeviceMap::const_iterator itr = device_map.begin();
	DeviceMap::const_iterator end = device_map.end();
	for (int n = 1 ; itr != end ; ++itr, ++n) {
		const char* uuid = (itr->second)->GetUUID().GetChars();
		const char* name = (itr->second)->GetFriendlyName().GetChars();
		list.push_back(DeviceInfo(uuid, name));
		printf("%d. %s\n", n, name);
	}
}



int main(void)
{
	PLT_UPnP upnp;

    ProxyCP* ctrlPoint = new ProxyCP(upnp);
    ctrlPoint->AddListener(ctrlPoint);

    PLT_CtrlPointReference ctrl(ctrlPoint);
    upnp.AddCtrlPoint(ctrl);
    upnp.Start();

    DisplayList display_list;

    char buf[256] = "0";
    do {
    	const int input_len = strlen(buf);

		if (input_len == 1 && *buf == 'q') {
			break;
		}

		if (input_len == 1 && *buf == '0') {
			// refresh
			display_list.clear();
			DisplayDevices(ctrlPoint->devices(), display_list);
			continue;
		}

		int selected;
		if (sscanf(buf, "%d", &selected) == 1 && selected > 0) {
			const DeviceInfo& info = display_list[selected-1];
			printf("[SELECTED] %s : %s\n", info.first.c_str(), info.second.c_str());
			ctrlPoint->StartDevice(info.first.c_str());
		} else {
			printf("[ERROR] Invalid input!! Try again.\n");
		}

    } while (gets(buf));

    upnp.Stop();
    return 0;
}
