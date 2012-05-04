#define BUILDING_NODE_EXTENSION

#include <iostream>
#include <string>
#include "HoneydProfileBinding.h"
#include "HoneydConfiguration.h"

using namespace std;
using namespace v8;
using namespace Nova;

HoneydProfileBinding::HoneydProfileBinding() {};

profile * HoneydProfileBinding::GetChild() {
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

	proto->Set("SetName",           FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetName>) );
	proto->Set("SetTcpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetTcpAction>) );
	proto->Set("SetUdpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetUdpAction>) );
	proto->Set("SetIcmpAction",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetIcmpAction>) );
	proto->Set("SetPersonality",    FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetPersonality>) );
	proto->Set("SetEthernet",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetEthernet>) );
	proto->Set("SetUptimeMin",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetUptimeMin>) );
	proto->Set("SetUptimeMax",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetUptimeMax>) );
	proto->Set("SetDropRate",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetDropRate>) );
	proto->Set("SetParentProfile",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, std::string, &Nova::profile::SetParentProfile>));
	
	proto->Set("setTcpActionInherited",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setTcpActionInherited>));
	proto->Set("setUdpActionInherited",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setUdpActionInherited>));
	proto->Set("setIcmpActionInherited", FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setIcmpActionInherited>));
	proto->Set("setPersonalityInherited",FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setPersonalityInherited>));
	proto->Set("setEthernetInherited",   FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setEthernetInherited>));
	proto->Set("setUptimeInherited",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setUptimeInherited>));
	proto->Set("setDropRateInherited",   FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, profile, bool, &Nova::profile::setDropRateInherited>));
	

    proto->Set(String::NewSymbol("Save"),FunctionTemplate::New(Save)->GetFunction()); 
    proto->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction()); 


	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydProfileBinding"), constructor);
}


v8::Handle<Value> HoneydProfileBinding::New(const Arguments& args)
{
	v8::HandleScope scope;

	HoneydProfileBinding* obj = new HoneydProfileBinding();
	obj->m_pfile = new profile();
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
