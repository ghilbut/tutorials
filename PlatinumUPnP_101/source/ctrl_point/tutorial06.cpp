#include <string>
#include <iostream>
#include "Platinum.h"



class TutorialCP : public PLT_CtrlPoint, public PLT_CtrlPointListener
{
public:
	TutorialCP(void)
	{
		AddListener(this);
	}
	~TutorialCP(void)
	{
	}

	virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device)
	{
		if (device->GetFriendlyName() == "Tutorial Device") {
			printf("[Tutorial Device] added\n");
			device->FindServiceById("urn:upnp-org:serviceId:Tutorial.001", service_);
			Subscribe(service_);
			device_ = device;
		}
		return NPT_SUCCESS;
	}

	virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device)
	{
		if (device->GetFriendlyName() == "Tutorial Device") {
			printf("[Tutorial Device] removed\n");
			service_ = 0;
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
		const PLT_DeviceData* device = service->GetDevice();
		if (device->GetFriendlyName() == "Tutorial Device") {
			NPT_List<PLT_StateVariable*>::Iterator itr = vars->GetFirstItem();
			for (bool stop = false ; !stop ; ++itr) {
				const PLT_StateVariable* state_variable = *itr;
				if (state_variable->GetName() == "PrintBuffer") {
					const NPT_String& value = state_variable->GetValue();
					printf("[PrintBuffer] changed: %s\n", value);
					break;
				}
				stop = (itr == vars->GetLastItem());
			}
		}
		return NPT_SUCCESS;
	}

	void Print(const char* str)
	{
		if (!device_.IsNull()) {
			PLT_ActionDesc* desc = service_->FindActionDesc("Print");
			PLT_ActionReference action(new PLT_Action(*desc));
			action->SetArgumentValue("String", str);
			InvokeAction(action);
		}
	}

private:
	PLT_DeviceDataReference device_;
	PLT_Service* service_;
};



int main()
{
	PLT_UPnP upnp;

	TutorialCP* ctrlpoint = new TutorialCP;
	PLT_CtrlPointReference ref(ctrlpoint);
	upnp.AddCtrlPoint(ref);
	upnp.Start();

	bool stop = false;
	do {
		std::string input;
		std::cin >> input;
		if (input == "q" || input == "Q") {
			stop = true;
		} else {
			ctrlpoint->Print(input.c_str());
		}
	} while(!stop);

	upnp.Stop();
	return 0;
}
