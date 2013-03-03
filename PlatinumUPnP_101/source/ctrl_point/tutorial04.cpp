#include <string>
#include <iostream>
#include "Platinum.h"



class TutorialListener : public PLT_CtrlPointListener
{
public:
	virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device)
	{
		return NPT_SUCCESS;
	}

	virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device)
	{
		return NPT_SUCCESS;
	}

	virtual NPT_Result OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata)
	{
		return NPT_SUCCESS;
	}

	virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars)
	{
		return NPT_SUCCESS;
	}
};



int main()
{
	PLT_UPnP upnp;

	TutorialListener listener;
	PLT_CtrlPointReference ctrlpoint(new PLT_CtrlPoint);
	ctrlpoint->AddListener(&listener);
	upnp.AddCtrlPoint(ctrlpoint);
	upnp.Start();

	bool stop = false;
	do {
		std::string input;
		std::cin >> input;
		if (input == "q" || input == "Q") {
			stop = true;
		} else {
			// nothing
		}
	} while(!stop);

	upnp.Stop();
	return 0;
}
