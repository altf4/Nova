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
	Profile profile = HoneydConfiguration::Inst()->Getprofile(profileName);
	if(profile == NULL)
	{
		//Then we need to make a brand new profile
		m_profile = new Profile(parentName, profileName);
		isNewProfile = true;
	}
	else
	{
		//There is already a profile with that name, so let's use it
		m_profile = profile;
		isNewProfile = false;
	}
}

HoneydProfileBinding::~HoneydProfileBinding()
{
}

bool HoneydProfileBinding::SetName(std::string name)
{
	m_profile->m_key = name;
	return true;
}

bool HoneydProfileBinding::SetPersonality(std::string personality)
{
	m_profile->m_personality = personality;
	return true;
}

bool HoneydProfileBinding::SetUptimeMin(uint uptimeMin)
{
	m_profile->m_uptimeMin = uptimeMin;
	return true;
}

bool HoneydProfileBinding::SetUptimeMax(uint uptimeMax)
{
	m_profile->m_uptimeMax = uptimeMax;
	return true;
}

bool HoneydProfileBinding::SetDropRate(std::string dropRate)
{
	m_profile->m_dropRate = dropRate;
	return true;
}

bool HoneydProfileBinding::SetParentProfile(std::string parentName)
{
	m_profile->m_parent = HoneydConfiguration::Inst()->Getprofile(parentName);
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
	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with at exactly two parameters")));
	}

	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());

	std::vector<std::string> ethVendors = cvv8::CastFromJS<std::vector<std::string> >(args[0]);
	std::vector<double> ethDists = cvv8::CastFromJS<std::vector<double> >(args[1]);

	if(ethVendors.size() != ethDists.size())
	{
		//Mismatch in sizes
		return scope.Close(Boolean::New(false));
	}

	if(ethVendors.size() == 0)
	{
		if(obj->m_profile == NULL)
		{
			return scope.Close(Boolean::New(false));
		}
		obj->m_profile->m_ethernetVendors.clear();
		return args.This();
	}
	else
	{
		std::vector<std::pair<std::string, double> > set;

		for(uint i = 0; i < ethVendors.size(); i++)
		{
			std::pair<std::string, double> vendorDists;
			vendorDists.first = ethVendors[i];
			vendorDists.second = ethDists[i];
			set.push_back(vendorDists);
		}

		return scope.Close(Boolean::New(obj->GetChild()->SetVendors(set)));
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
	port.m_protocol = port::StringToPortProtocol(portProtcol);
	port.m_portNumber = portNumber;
	if(port.m_behavior == PORT_SCRIPT || port.m_behavior == PORT_TARPIT_SCRIPT)
	{
		port.m_scriptName = portScriptName;
	}

	portSet->AddPort(port);

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydProfileBinding::AddPortSet(const Arguments& args)
{
	if( args.Length() != 1 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());
	if(obj == NULL)
	{
		return scope.Close(Boolean::New(false));
	}

	//TODO: Test for uniqueness
	string portSetName = cvv8::CastFromJS<string>( args[0] );

	obj->m_profile->m_portSets.push_back(new PortSet(portSetName));

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydProfileBinding::ClearPorts(const Arguments& args)
{
	if( args.Length() != 0 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with no parameters")));
	}

	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());
	if(obj == NULL)
	{
		return scope.Close(Boolean::New(false));
	}

	string portSetName = cvv8::CastFromJS<string>( args[0] );

	//Delete the port sets, then clear the list of them
	for(uint i = 0; i < obj->m_profile->m_portSets.size(); i++)
	{
		delete obj->m_profile->m_portSets[i];
	}
	obj->m_profile->m_portSets.clear();

	return scope.Close(Boolean::New(true));
}

void HoneydProfileBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydProfileBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	Local<Template> proto = tpl->PrototypeTemplate();

	proto->Set("SetName",			FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::HoneydProfileBinding::SetName>));
	proto->Set("SetPersonality",	FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::HoneydProfileBinding::SetPersonality>));
	proto->Set("SetUptimeMin",		FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, uint, 		&Nova::HoneydProfileBinding::SetUptimeMin>));
	proto->Set("SetUptimeMax",		FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, uint, 		&Nova::HoneydProfileBinding::SetUptimeMax>));
	proto->Set("SetDropRate",		FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::HoneydProfileBinding::SetDropRate>));
	proto->Set("SetParentProfile",	FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::HoneydProfileBinding::SetParentProfile>));
	proto->Set("SetCount",			FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, int, 		&Nova::HoneydProfileBinding::SetCount>));
	proto->Set("SetIsGenerated",	FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, 		&Nova::HoneydProfileBinding::SetIsGenerated>));

	proto->Set(String::NewSymbol("SetVendors"),FunctionTemplate::New(SetVendors)->GetFunction());

    proto->Set(String::NewSymbol("Save"),FunctionTemplate::New(Save)->GetFunction());
    proto->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction());
    proto->Set(String::NewSymbol("AddPortSet"),FunctionTemplate::New(AddPortSet)->GetFunction());

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

Handle<Value> HoneydProfileBinding::Save(const Arguments& args)
{

	HandleScope scope;
	HoneydProfileBinding* newProfile = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());
	string profileName = newProfile->m_profile->m_name;

	//If this is a new profile, then add it in
	if(newProfile->m_isNewProfile)
	{
		HoneydConfiguration::Inst()->AddProfile(newProfile->m_profile);
	}
	
	HoneydConfiguration::Inst()->UpdateNodes(profileName);

	HoneydConfiguration::Inst()->WriteAllTemplatesToXML();
	HoneydConfiguration::Inst()->WriteHoneydConfiguration();
	
	return scope.Close(Boolean::New(true));
}
