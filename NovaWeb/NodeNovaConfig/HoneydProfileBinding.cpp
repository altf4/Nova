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
	delete m_pfile;
}

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

	proto->Set("SetName",           FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetName>));
	proto->Set("SetTcpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetTcpAction>));
	proto->Set("SetUdpAction",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetUdpAction>));
	proto->Set("SetIcmpAction",     FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetIcmpAction>));
	proto->Set("SetPersonality",    FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetPersonality>));
	proto->Set("SetEthernet",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetEthernet>));
	proto->Set("SetUptimeMin",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetUptimeMin>));
	proto->Set("SetUptimeMax",      FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetUptimeMax>));
	proto->Set("SetDropRate",       FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetDropRate>));
	proto->Set("SetParentProfile",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, std::string, &Nova::NodeProfile::SetParentProfile>));
	proto->Set("SetDistribution",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, double, &Nova::NodeProfile::SetDistribution>));
	proto->Set("SetGenerated",  FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, NodeProfile, bool, &Nova::NodeProfile::SetGenerated>));
	proto->Set(String::NewSymbol("SetVendors"),FunctionTemplate::New(SetVendors)->GetFunction());
	
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

	HoneydConfiguration *conf = new HoneydConfiguration();

	conf->LoadAllTemplates();
	
	if(isNewProfile)
	{
		conf->AddProfile(newProfile->m_pfile);
	}
	else
	{
		conf->m_profiles[oldName].SetTcpAction(newProfile->m_pfile->m_tcpAction);
		conf->m_profiles[oldName].SetUdpAction(newProfile->m_pfile->m_udpAction);
		conf->m_profiles[oldName].SetIcmpAction(newProfile->m_pfile->m_icmpAction);
		conf->m_profiles[oldName].SetPersonality(newProfile->m_pfile->m_personality);
		
		conf->m_profiles[oldName].SetUptimeMin(newProfile->m_pfile->m_uptimeMin);
		conf->m_profiles[oldName].SetUptimeMax(newProfile->m_pfile->m_uptimeMax);
		
		conf->m_profiles[oldName].SetDropRate(newProfile->m_pfile->m_dropRate);
		conf->m_profiles[oldName].SetParentProfile(newProfile->m_pfile->m_parentProfile);
		
		conf->m_profiles[oldName].SetVendors(newProfile->m_pfile->m_ethernetVendors);
		
		conf->m_profiles[oldName].setTcpActionInherited(newProfile->m_pfile->isTcpActionInherited());
		conf->m_profiles[oldName].setUdpActionInherited(newProfile->m_pfile->isUdpActionInherited());
		conf->m_profiles[oldName].setIcmpActionInherited(newProfile->m_pfile->isIcmpActionInherited());
		conf->m_profiles[oldName].setPersonalityInherited(newProfile->m_pfile->isPersonalityInherited());
		conf->m_profiles[oldName].setEthernetInherited(newProfile->m_pfile->isEthernetInherited());
		conf->m_profiles[oldName].setUptimeInherited(newProfile->m_pfile->isUptimeInherited());
		conf->m_profiles[oldName].setDropRateInherited(newProfile->m_pfile->isDropRateInherited());
		
		conf->m_profiles[oldName].SetGenerated(newProfile->m_pfile->m_generated);
		conf->m_profiles[oldName].SetDistribution(newProfile->m_pfile->m_distribution);
	
		std::vector<std::string> portNames = conf->m_profiles[oldName].GetPortNames();
	
		// Delete old ports
		conf->m_profiles[oldName].m_ports.clear();

		for(uint i = 0; i < newProfile->m_pfile->m_ports.size(); i++)
		{
			conf->m_profiles[oldName].m_ports.push_back(newProfile->m_pfile->m_ports[i]);
		}
		
		conf->UpdateProfile("default");
	
		if(oldName.compare(newProfile->m_pfile->m_name)) 
		{
			if(!conf->RenameProfile(oldName, newProfile->m_pfile->m_name))
			{
				std::cout << "Couldn't rename profile " << oldName << " to " << newProfile->m_pfile->m_name << std::endl;
			}
		}
		
		conf->UpdateNodeMacs(newProfile->m_pfile->m_name);
		
		conf->UpdateProfile(newProfile->m_pfile->m_parentProfile);
	}
	
	conf->SaveAllTemplates();
	conf->WriteHoneydConfiguration();

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

	string portName = cvv8::CastFromJS<string>( args[0] );
	bool isInherited = cvv8::CastFromJS<bool>( args[1] );

	return scope.Close(Boolean::New(obj->GetChild()->AddPort(portName, isInherited, 100)));
}
