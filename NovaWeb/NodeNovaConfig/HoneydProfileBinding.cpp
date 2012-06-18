#define BUILDING_NODE_EXTENSION

#include <iostream>
#include <string>
#include "HoneydProfileBinding.h"
#include "HoneydConfiguration.h"

using namespace std;
using namespace v8;
using namespace Nova;

HoneydProfileBinding::HoneydProfileBinding() {};

NodeProfile *HoneydProfileBinding::GetChild() {
	return m_pfile;
}

void HoneydProfileBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydProfileBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	Local<Template> proto = tpl->PrototypeTemplate();

	proto->Set("SetName",           FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetName>) );
	proto->Set("SetTcpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetTcpAction>) );
	proto->Set("SetUdpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetUdpAction>) );
	proto->Set("SetIcmpAction",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetIcmpAction>) );
	proto->Set("SetPersonality",    FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetPersonality>) );
	proto->Set("SetEthernet",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetEthernet>) );
	proto->Set("SetUptimeMin",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetUptimeMin>) );
	proto->Set("SetUptimeMax",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetUptimeMax>) );
	proto->Set("SetDropRate",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetDropRate>) );
	proto->Set("SetParentProfile",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetParentProfile>));
	
	proto->Set("setTcpActionInherited",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setTcpActionInherited>));
	proto->Set("setUdpActionInherited",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setUdpActionInherited>));
	proto->Set("setIcmpActionInherited", FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setIcmpActionInherited>));
	proto->Set("setPersonalityInherited",FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setPersonalityInherited>));
	proto->Set("setEthernetInherited",   FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setEthernetInherited>));
	proto->Set("setUptimeInherited",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setUptimeInherited>));
	proto->Set("setDropRateInherited",   FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::setDropRateInherited>));
	

    proto->Set(String::NewSymbol("Save"),FunctionTemplate::New(Save)->GetFunction()); 
    proto->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction()); 


	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydProfileBinding"), constructor);
}


v8::Handle<Value> HoneydProfileBinding::New(const Arguments& args)
{
	v8::HandleScope scope;

	HoneydProfileBinding* obj = new HoneydProfileBinding();
	obj->m_pfile = new NodeProfile();
	obj->Wrap(args.This());

	return args.This();
}



Handle<Value> HoneydProfileBinding::Save(const Arguments& args)
{
	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());
	
	HoneydConfiguration *conf = new HoneydConfiguration();

	conf->LoadAllTemplates();
	conf->AddProfile(obj->m_pfile);

	conf->SaveAllTemplates();

	delete conf;
	return scope.Close(Boolean::New(true));
}


Handle<Value> HoneydProfileBinding::AddPort(const Arguments& args) 
{
	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());
    
	if( args.Length() != 2 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
    }

    std::string portName = cvv8::CastFromJS<std::string>( args[0] );
    bool isInherited = cvv8::CastFromJS<bool>( args[1] );

	return scope.Close(Boolean::New(obj->GetChild()->AddPort(portName, isInherited)));
}
