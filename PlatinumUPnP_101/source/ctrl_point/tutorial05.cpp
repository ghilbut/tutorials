#include <string>
#include <iostream>
#include "Platinum.h"



class TutorialListener : public PLT_CtrlPointListener
{
public:
	virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device)
	{
		if (device->GetFriendlyName() == "Tutorial Device") {
			printf("[Tutorial Device] added\n");
			device_ = device;
		}
		return NPT_SUCCESS;
	}

	virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device)
	{
		if (device->GetFriendlyName() == "Tutorial Device") {
			printf("[Tutorial Device] removed\n");
			device_ = 0;
		}
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

	PLT_DeviceData* device(void) const
	{
		return device_.AsPointer();
	}

private:
	PLT_DeviceDataReference device_;
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
			PLT_DeviceData* device = listener.device();
			if (device != 0) {
				PLT_Service* service;
				device->FindServiceById("urn:upnp-org:serviceId:Tutorial.001", service);
				PLT_ActionDesc* desc = service->FindActionDesc("Print");
				PLT_ActionReference action(new PLT_Action(*desc));
				action->SetArgumentValue("String", input.c_str());
				ctrlpoint->InvokeAction(action);
			}
		}
	} while(!stop);

	upnp.Stop();
	return 0;
}
