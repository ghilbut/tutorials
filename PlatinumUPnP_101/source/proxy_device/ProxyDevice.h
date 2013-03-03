#ifndef PROXY_DEVICE_H_
#define PROXY_DEVICE_H_

#include <map>
#include <string>
#include "Platinum.h"



typedef std::map<std::string, PLT_DeviceDataReference> DeviceMap;

class ProxyCP : public PLT_CtrlPoint, 
				public PLT_CtrlPointListener
{
public:
	explicit ProxyCP(PLT_UPnP& upnp);
	~ProxyCP(void);

public:
    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device);
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device);
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata);
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars);

	virtual void OnSetupService(PLT_DeviceHost* device);
	virtual NPT_Result OnAction(PLT_ActionReference& action, const PLT_HttpRequestContext& context);

    void StartDevice(std::string uuid);
	void StopDevice(void);

	const DeviceMap& devices() const;

private:
	PLT_ActionReference CloneActionForStubDevice(PLT_Action& proxy_action) const;

private:
	PLT_UPnP& upnp_;
    DeviceMap devices_;
	PLT_DeviceDataReference stub_device_;
	PLT_DeviceHostReference proxy_device_;
	NPT_SharedVariable wait_;
};



class ProxyDevice : public PLT_DeviceHost
{
public:
	explicit ProxyDevice(ProxyCP& ctrl_point);
	~ProxyDevice(void);

	virtual NPT_Result SetupServices(void);
	virtual NPT_Result OnAction(PLT_ActionReference& action, const PLT_HttpRequestContext& context);

private:
	ProxyCP& ctrl_point_;
};

#endif // PROXY_DEVICE_H_