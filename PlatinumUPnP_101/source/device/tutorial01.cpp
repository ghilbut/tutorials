#include <stdio.h>
#include "Platinum.h"



class TutorialDevice : public PLT_DeviceHost
{
public:
	TutorialDevice()
		: PLT_DeviceHost()
	{
		m_FriendlyName = "Tutorial Device";
	}
	~TutorialDevice()
	{
	}

protected:
	virtual NPT_Result SetupServices(void)
	{
		return NPT_SUCCESS;
	}
};



int main()
{
	PLT_UPnP upnp;

	PLT_DeviceHostReference device(new TutorialDevice());
	upnp.AddDevice(device);
	upnp.Start();

	bool stop = false;
	do {
		const char c = fgetc(stdin);
		stop = (c == 'q' || c == 'Q');
	} while(!stop);

	upnp.Stop();
	return 0;
}
