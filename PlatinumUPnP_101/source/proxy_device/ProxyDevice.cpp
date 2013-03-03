#include "ProxyDevice.h"



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BEGIN: Utilities

void CopyActionArguements(PLT_Action& from, PLT_Action& to)
{
	PLT_ActionDesc& desc = from.GetActionDesc();
	NPT_Array<PLT_ArgumentDesc*>& args = desc.GetArgumentDescs();
	NPT_Array<PLT_ArgumentDesc*>::Iterator itr = args.GetFirstItem();
	for (bool stop = false ; !stop ; ++itr) {
		PLT_ArgumentDesc* arg = *itr;
		const char* name = arg->GetName();
		NPT_String value;
		from.GetArgumentValue(name, value);
		to.SetArgumentValue(name, value);
		stop = (itr == args.GetLastItem());
	}
}

// END: Utilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



ProxyCP::ProxyCP(PLT_UPnP& upnp)
	: PLT_CtrlPoint(), upnp_(upnp)
{
}
ProxyCP::~ProxyCP(void)
{
}

NPT_Result ProxyCP::OnDeviceAdded(PLT_DeviceDataReference& device)
{
	devices_[device->GetUUID().GetChars()] = device;
	return NPT_SUCCESS;
}

NPT_Result ProxyCP::OnDeviceRemoved(PLT_DeviceDataReference& device)
{
	const std::string key = device->GetUUID().GetChars();
	DeviceMap::iterator itr = devices_.find(key);
	if(itr != devices_.end()) {
		devices_.erase(key);
	}
	return NPT_SUCCESS;
}

NPT_Result ProxyCP::OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata)
{
	wait_.SetValue(1);
	return NPT_SUCCESS;
}

NPT_Result ProxyCP::OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars)
{
	NPT_List<PLT_StateVariable*>::Iterator itr = vars->GetFirstItem();
	if (vars->GetItemCount() > 0) {
		for (bool stop = false ; !stop ; ++itr) {
			PLT_StateVariable* stub_variable = *itr;
			PLT_Service* stub_service = stub_variable->GetService();

			PLT_Service* proxy_service;
			proxy_device_->FindServiceById(stub_service->GetServiceID(), proxy_service);
			proxy_service->SetStateVariable(stub_variable->GetName(), stub_variable->GetValue());

			stop = (itr == vars->GetLastItem());
		}
	}
	return NPT_SUCCESS;
}

void ProxyCP::OnSetupService(PLT_DeviceHost* device)
{
	const NPT_Array<PLT_Service*>& services = stub_device_->GetServices();

	const int count = services.GetItemCount();
	for (int index = 0 ; index < count ; ++index) {
		PLT_Service* stub_service = services[index];
		Subscribe(stub_service);

		NPT_String xml;
		stub_service->GetSCPDXML(xml);
		PLT_Service* service = new PLT_Service(proxy_device_.AsPointer(), 
												stub_service->GetServiceType(), 
												stub_service->GetServiceID(), 
												stub_service->GetServiceName());
		service->SetSCPDXML(xml);
		proxy_device_->AddService(service);

		//service->SetStateVariable("Status", "True");
	}
}

NPT_Result ProxyCP::OnAction(PLT_ActionReference& action, const PLT_HttpRequestContext& context)
{
	NPT_COMPILER_UNUSED(context);

	NPT_Result result = NPT_FAILURE;
	PLT_ActionReference stub = CloneActionForStubDevice(*action);
	if (!stub.IsNull()) {
		result = InvokeAction(stub);
		wait_.WaitUntilEquals(1);
		wait_.SetValue(0);

		unsigned int e_code;
		const char* e_desc = stub->GetError(&e_code);
		action->SetError(e_code, e_desc);
		CopyActionArguements(*stub, *action);
	}
	return result;
}

void ProxyCP::StartDevice(std::string uuid)
{
	stub_device_ = devices_[uuid];
	proxy_device_ = new ProxyDevice(*this);
	upnp_.AddDevice(proxy_device_);
}

void ProxyCP::StopDevice(void)
{
	upnp_.RemoveDevice(proxy_device_);
	proxy_device_ = 0;
	stub_device_ = 0;
}

const DeviceMap& ProxyCP::devices() const
{
	return devices_;
}

PLT_ActionReference ProxyCP::CloneActionForStubDevice(PLT_Action& proxy_action) const
{
	PLT_ActionDesc& proxy_desc = proxy_action.GetActionDesc();
	PLT_Service* proxy_service = proxy_desc.GetService();

	PLT_Service* stub_service;
	stub_device_->FindServiceById(proxy_service->GetServiceID(), stub_service);
	PLT_ActionDesc* stub_desc = stub_service->FindActionDesc(proxy_desc.GetName());
	PLT_ActionReference stub_action(new PLT_Action(*stub_desc));
	CopyActionArguements(proxy_action, *stub_action);
	return stub_action;
}



ProxyDevice::ProxyDevice(ProxyCP& ctrl_point)
	: PLT_DeviceHost("/", "", "urn:schemas-upnp-org:device:ProxyDevice:1", "Proxy Device"), ctrl_point_(ctrl_point)
{
}

ProxyDevice::~ProxyDevice(void)
{
}

NPT_Result ProxyDevice::SetupServices(void)
{
	ctrl_point_.OnSetupService(this);
	return NPT_SUCCESS;
}

NPT_Result ProxyDevice::OnAction(PLT_ActionReference& action, const PLT_HttpRequestContext& context)
{
	return ctrl_point_.OnAction(action, context);
}
