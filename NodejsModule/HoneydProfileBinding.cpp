#define BUILDING_NODE_EXTENSION

#include <iostream>
#include <string>
#include "HoneydProfileBinding.h"
#include "HoneydConfiguration/HoneydConfiguration.h"

using namespace std;
using namespace v8;
using namespace Nova;

HoneydProfileBinding::HoneydProfileBinding()
{
	m_pfile = NULL;
}

HoneydProfileBinding::~HoneydProfileBinding()
{
}

Profile *HoneydProfileBinding::GetChild() {
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

	proto->Set("SetName",           FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetName>));
	proto->Set("SetTcpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetTcpAction>));
	proto->Set("SetUdpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetUdpAction>));
	proto->Set("SetIcmpAction",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetIcmpAction>));
	proto->Set("SetPersonality",    FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetPersonality>));
	proto->Set("SetEthernet",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetEthernet>));
	proto->Set("SetUptimeMin",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetUptimeMin>));
	proto->Set("SetUptimeMax",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetUptimeMax>));
	proto->Set("SetDropRate",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetDropRate>));
	proto->Set("SetParentProfile",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, std::string, &Nova::Profile::SetParentProfile>));
	proto->Set("SetDistribution",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, double, &Nova::Profile::SetDistribution>));
	proto->Set("SetGenerated",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::SetGenerated>));
	proto->Set(String::NewSymbol("SetVendors"),FunctionTemplate::New(SetVendors)->GetFunction());
	
	proto->Set("setTcpActionInherited",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setTcpActionInherited>));
	proto->Set("setUdpActionInherited",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setUdpActionInherited>));
	proto->Set("setIcmpActionInherited", FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setIcmpActionInherited>));
	proto->Set("setPersonalityInherited",FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setPersonalityInherited>));
	proto->Set("setEthernetInherited",   FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setEthernetInherited>));
	proto->Set("setUptimeInherited",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setUptimeInherited>));
	proto->Set("setDropRateInherited",   FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, bool, &Nova::Profile::setDropRateInherited>));

    proto->Set(String::NewSymbol("Save"),FunctionTemplate::New(Save)->GetFunction()); 
    proto->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction()); 


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

	v8::HandleScope scope;

	HoneydProfileBinding* obj = new HoneydProfileBinding();
	obj->m_pfile = new Profile(parentName, profileName);
	obj->Wrap(args.This());

	return args.This();
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

	std::vector<std::pair<std::string, double> > set;

	if(ethVendors.size() == 0)
	{
		HoneydConfiguration *conf = new HoneydConfiguration();
		conf->LoadAllTemplates();
		std::cout << "clearing m_ethernetVendors." << std::endl;
		obj->m_pfile->m_ethernetVendors.clear();
		std::cout << "m_ethernetVendors.size() == " << obj->m_pfile->m_ethernetVendors.size() << std::endl;
		conf->m_profiles[obj->m_pfile->m_name].m_ethernetVendors.clear();
		conf->UpdateProfile(obj->m_pfile->m_name);
		conf->SaveAllTemplates();
		conf->WriteHoneydConfiguration();
		return args.This();
	}
	else
	{
		for(uint i = 0; i < ethVendors.size(); i++)
		{
		std::pair<std::string, double> vendorDists;
		vendorDists.first = ethVendors[i];
		vendorDists.second = ethDists[i];
		set.push_back(vendorDists);
		}

		for(uint i = 0; i < set.size(); i++)
		{
			std::cout << "set[" << i << "] = {" << set[i].first << ", " << set[i].second << "}\n";
		}

		return scope.Close(Boolean::New(obj->GetChild()->SetVendors(set)));
	}
}

Handle<Value> HoneydProfileBinding::Save(const Arguments& args)
{

    if(args.Length() != 2)
    {
      return ThrowException(Exception::TypeError(String::New("Must be invoked with at exactly two parameters")));
    }
    
	HandleScope scope;
	HoneydProfileBinding* newProfile = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());

	std::string oldName = cvv8::CastFromJS<std::string>(args[0]);
	
	bool isNewProfile = cvv8::CastFromJS<bool>(args[1]);
	
	if(isNewProfile)
	{
		HoneydConfiguration::Inst()->AddProfile(newProfile->m_pfile);
	}
	else
	{
		HoneydConfiguration::Inst()->m_profiles[oldName].SetTcpAction(newProfile->m_pfile->m_tcpAction);
		HoneydConfiguration::Inst()->m_profiles[oldName].SetUdpAction(newProfile->m_pfile->m_udpAction);
		HoneydConfiguration::Inst()->m_profiles[oldName].SetIcmpAction(newProfile->m_pfile->m_icmpAction);
		HoneydConfiguration::Inst()->m_profiles[oldName].SetPersonality(newProfile->m_pfile->m_personality);
		
		HoneydConfiguration::Inst()->m_profiles[oldName].SetUptimeMin(newProfile->m_pfile->m_uptimeMin);
		HoneydConfiguration::Inst()->m_profiles[oldName].SetUptimeMax(newProfile->m_pfile->m_uptimeMax);
		
		HoneydConfiguration::Inst()->m_profiles[oldName].SetDropRate(newProfile->m_pfile->m_dropRate);
		HoneydConfiguration::Inst()->m_profiles[oldName].SetParentProfile(newProfile->m_pfile->m_parentProfile);
		
		HoneydConfiguration::Inst()->m_profiles[oldName].SetVendors(newProfile->m_pfile->m_ethernetVendors);
		
		HoneydConfiguration::Inst()->m_profiles[oldName].setTcpActionInherited(newProfile->m_pfile->isTcpActionInherited());
		HoneydConfiguration::Inst()->m_profiles[oldName].setUdpActionInherited(newProfile->m_pfile->isUdpActionInherited());
		HoneydConfiguration::Inst()->m_profiles[oldName].setIcmpActionInherited(newProfile->m_pfile->isIcmpActionInherited());
		HoneydConfiguration::Inst()->m_profiles[oldName].setPersonalityInherited(newProfile->m_pfile->isPersonalityInherited());
		HoneydConfiguration::Inst()->m_profiles[oldName].setEthernetInherited(newProfile->m_pfile->isEthernetInherited());
		HoneydConfiguration::Inst()->m_profiles[oldName].setUptimeInherited(newProfile->m_pfile->isUptimeInherited());
		HoneydConfiguration::Inst()->m_profiles[oldName].setDropRateInherited(newProfile->m_pfile->isDropRateInherited());
		
		HoneydConfiguration::Inst()->m_profiles[oldName].SetGenerated(newProfile->m_pfile->m_generated);
		conf->m_profiles[oldName].SetDistribution(newProfile->m_pfile->m_distribution);
	
		std::vector<std::string> portNames = HoneydConfiguration::Inst()->m_profiles[oldName].GetPortNames();
	
		// Delete old ports
		HoneydConfiguration::Inst()->m_profiles[oldName].m_ports.clear();

		for(uint i = 0; i < newProfile->m_pfile->m_ports.size(); i++)
		{
			HoneydConfiguration::Inst()->m_profiles[oldName].m_ports.push_back(newProfile->m_pfile->m_ports[i]);
		}
		
		conf->UpdateProfile("default");
	
		if(oldName.compare(newProfile->m_pfile->m_name)) 
		{
			if(!HoneydConfiguration::Inst()->RenameProfile(oldName, newProfile->m_pfile->m_name))
			{
				std::cout << "Couldn't rename profile " << oldName << " to " << newProfile->m_pfile->m_name << std::endl;
			}
		}
		
		HoneydConfiguration::Inst()->UpdateNodeMacs(newProfile->m_pfile->m_name);
		
		HoneydConfiguration::Inst()->UpdateProfile(newProfile->m_pfile->m_parentProfile);
	}
	
	HoneydConfiguration::Inst()->SaveAllTemplates();
	HoneydConfiguration::Inst()->WriteHoneydConfiguration();

	delete HoneydConfiguration::Inst();
	
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

	string portName = cvv8::CastFromJS<string>( args[0] );
	bool isInherited = cvv8::CastFromJS<bool>( args[1] );

	return scope.Close(Boolean::New(obj->GetChild()->AddPort(portName, isInherited, 100)));
}
