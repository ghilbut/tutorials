#include <string>
#include <iostream>
#include "Platinum.h"



const char* SCPDXML_TUTORIAL =
	"<?xml version=\"1.0\" ?>"
	"  <scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">"
	"    <specVersion>"
	"       <major>1</major>"
	"	    <minor>0</minor>"
	"	 </specVersion>"
	"    <serviceStateTable>"
	"      <stateVariable sendEvents=\"yes\">"
	"        <name>PrintBuffer</name>"
	"        <dataType>string</dataType>"
	"        <defaultValue></defaultValue>"
	"      </stateVariable>"
	"    </serviceStateTable>"
	"  </scpd>";



class TutorialDevice : public PLT_DeviceHost
{
public:
	TutorialDevice()
		: PLT_DeviceHost("/", "", "urn:schemas-upnp-org:device:Tutorial:1", "Tutorial Device")
	{
	}
	~TutorialDevice()
	{
	}

	void SetPrintBuffer(const char* str)
	{
		PLT_Service* service;
		if (FindServiceByName("Tutorial", service) == NPT_SUCCESS) {
			service->SetStateVariable("PrintBuffer", str);
		}
	}

protected:
	virtual NPT_Result SetupServices(void)
	{
		PLT_Service* service = new PLT_Service(
			this,
			"urn:schemas-upnp-org:service:Tutorial:1", 
			"urn:upnp-org:serviceId:Tutorial.001",
			"Tutorial");
		service->SetSCPDXML(SCPDXML_TUTORIAL);
		AddService(service);

		service->SetStateVariable("Status", "True");

		return NPT_SUCCESS;
	}
};



int main()
{
	PLT_UPnP upnp;

	TutorialDevice* tutorial_device = new TutorialDevice();
	PLT_DeviceHostReference device(tutorial_device);
	upnp.AddDevice(device);
	upnp.Start();

	bool stop = false;
	do {
		std::string input;
		std::cin >> input;
		if (input == "q" || input == "Q") {
			stop = true;
		} else {
			tutorial_device->SetPrintBuffer(input.c_str());
		}
	} while(!stop);

	upnp.Stop();
	return 0;
}
