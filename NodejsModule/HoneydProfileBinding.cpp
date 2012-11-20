#define BUILDING_NODE_EXTENSION

#include <iostream>
#include <string>
#include "HoneydProfileBinding.h"
#include "HoneydConfiguration/HoneydConfiguration.h"

using namespace std;
using namespace v8;
using namespace Nova;

HoneydProfileBinding::HoneydProfileBinding(string parentName, string profileName)
{
	Profile *profile = HoneydConfiguration::Inst()->GetProfile(profileName);
	if(profile == NULL)
	{
		//Then we need to make a brand new profile
		m_profile = new Profile(parentName, profileName);
		HoneydConfiguration::Inst()->AddProfile(m_profile);
	}
	else
	{
		//There is already a profile with that name, so let's use it
		m_profile = profile;
	}
}

HoneydProfileBinding::~HoneydProfileBinding()
{
}

bool HoneydProfileBinding::SetName(std::string name)
{
	m_profile->m_name = name;
	return true;
}

bool HoneydProfileBinding::SetPersonality(std::string personality)
{
	m_profile->SetPersonality(personality);
	return true;
}

bool HoneydProfileBinding::SetUptimeMin(uint uptimeMin)
{
	m_profile->SetUptimeMin(uptimeMin);
	return true;
}

bool HoneydProfileBinding::SetUptimeMax(uint uptimeMax)
{
	m_profile->SetUptimeMax(uptimeMax);
	return true;
}

bool HoneydProfileBinding::SetDropRate(std::string dropRate)
{
	m_profile->SetDropRate(dropRate);
	return true;
}

bool HoneydProfileBinding::SetParentProfile(std::string parentName)
{
	m_profile->m_parent = HoneydConfiguration::Inst()->GetProfile(parentName);
	return true;
}

bool HoneydProfileBinding::SetCount(int count)
{
	m_profile->m_count = count;
	return true;
}

bool HoneydProfileBinding::SetIsGenerated(bool isGenerated)
{
	m_profile->m_isGenerated = isGenerated;
	return true;
}

Profile *HoneydProfileBinding::GetChild()
{
	return m_profile;
}

Handle<Value> HoneydProfileBinding::SetVendors(const Arguments& args)
{
	HandleScope scope;
	if( args.Length() != 2 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
	}

	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());
	if(obj == NULL)
	{
		return scope.Close(Boolean::New(false));
	}

	vector<string> vendorNames = cvv8::CastFromJS<vector<string>>( args[0] );
	vector<uint> vendorCount = cvv8::CastFromJS<vector<uint>>( args[1] );

	if(vendorNames.size() != vendorCount.size())
	{
		//Mismatch in sizes
		return scope.Close(Boolean::New(false));
	}

	if(vendorNames.size() == 0)
	{
		obj->m_profile->m_vendors.clear();
		return scope.Close(Boolean::New(true));
	}
	else
	{
		std::vector<std::pair<std::string, uint> > set;

		for(uint i = 0; i < vendorNames.size(); i++)
		{
			std::pair<std::string, uint> vendor;
			vendor.first = vendorNames[i];
			vendor.second = vendorCount[i];
			set.push_back(vendor);
		}
		obj->m_profile->m_vendors = set;
		return scope.Close(Boolean::New(true));
	}
}

Handle<Value> HoneydProfileBinding::AddPort(const Arguments& args)
{
	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with five parameters")));
	}

	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());

	if(obj == NULL)
	{
		return scope.Close(Boolean::New(false));
	}

	string portSetName = cvv8::CastFromJS<string>( args[0] );
	string portBehavior = cvv8::CastFromJS<string>( args[1] );
	string portProtcol = cvv8::CastFromJS<string>( args[2] );
	uint portNumber = cvv8::CastFromJS<uint>( args[3] );
	string portScriptName = cvv8::CastFromJS<string>( args[4] );

	PortSet *portSet = obj->m_profile->GetPortSet(portSetName);
	if(portSet == NULL)
	{
		return scope.Close(Boolean::New(false));
	}

	Port port;
	port.m_behavior = Port::StringToPortBehavior(portBehavior);
	port.m_protocol = Port::StringToPortProtocol(portProtcol);
	port.m_portNumber = portNumber;
	if(port.m_behavior == PORT_SCRIPT || port.m_behavior == PORT_TARPIT_SCRIPT)
	{
		port.m_scriptName = portScriptName;
	}

	portSet->AddPort(port);

	return scope.Close(Boolean::New(true));
}

bool HoneydProfileBinding::AddPortSet(std::string portSetName)
{
	if(m_profile->GetPortSet(portSetName) != NULL)
	{
		return false;
	}

	m_profile->m_portSets.push_back(new PortSet(portSetName));

	return true;
}

bool HoneydProfileBinding::ClearPorts()
{
	//Delete the port sets, then clear the list of them
	for(uint i = 0; i < m_profile->m_portSets.size(); i++)
	{
		delete m_profile->m_portSets[i];
	}
	m_profile->m_portSets.clear();

	return true;
}

void HoneydProfileBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydProfileBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	Local<Template> proto = tpl->PrototypeTemplate();

	proto->Set("SetName",			FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  std::string, &HoneydProfileBinding::SetName>));
	proto->Set("SetPersonality",	FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  std::string, &HoneydProfileBinding::SetPersonality>));
	proto->Set("SetUptimeMin",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  uint, 		&HoneydProfileBinding::SetUptimeMin>));
	proto->Set("SetUptimeMax",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  uint, 		&HoneydProfileBinding::SetUptimeMax>));
	proto->Set("SetDropRate",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  std::string, &HoneydProfileBinding::SetDropRate>));
	proto->Set("SetParentProfile",	FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  std::string, &HoneydProfileBinding::SetParentProfile>));
	proto->Set("SetCount",			FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  int, 		&HoneydProfileBinding::SetCount>));
	proto->Set("SetIsGenerated",	FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  bool, 		&HoneydProfileBinding::SetIsGenerated>));
	proto->Set("AddPortSet",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  std::string, &HoneydProfileBinding::AddPortSet>));
	proto->Set("ClearPorts",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  &HoneydProfileBinding::ClearPorts>));

	//Odd ball out, because it needs 5 parameters. More than InvoleWrappedMethod can handle
	proto->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction());
	proto->Set(String::NewSymbol("SetVendors"),FunctionTemplate::New(SetVendors)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydProfileBinding"), constructor);
}


v8::Handle<Value> HoneydProfileBinding::New(const Arguments& args)
{
	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with at exactly two parameters")));
	}

	std::string parentName = cvv8::CastFromJS<std::string>(args[0]);
	std::string profileName = cvv8::CastFromJS<std::string>(args[1]);

	HoneydProfileBinding* binding = new HoneydProfileBinding(parentName, profileName);
	binding->Wrap(args.This());
	return args.This();
}
