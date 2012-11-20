#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "HoneydConfigBinding.h"
#include "HoneydTypesJs.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

HoneydConfigBinding::HoneydConfigBinding()
{
	m_conf = NULL;
};

HoneydConfigBinding::~HoneydConfigBinding()
{
	delete m_conf;
};

HoneydConfiguration *HoneydConfigBinding::GetChild()
{
	return m_conf;
}

void HoneydConfigBinding::Init(Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydConfigBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::ReadAllTemplatesXML>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::WriteAllTemplatesToXML>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteHoneydConfiguration"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::WriteHoneydConfiguration>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGeneratedProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGeneratedProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNodeMACs"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetNodeMACs >));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetScriptNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetScriptNames>));

	//tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteProfile>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteNode"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteNode>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNodes"),FunctionTemplate::New(AddNodes)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNode"),FunctionTemplate::New(GetNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNode"),FunctionTemplate::New(AddNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfile"),FunctionTemplate::New(GetProfile)->GetFunction());
	//TODO
	//tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPorts"),FunctionTemplate::New(GetPorts)->GetFunction());
	//tpl->PrototypeTemplate()->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddScript"),FunctionTemplate::New(AddScript)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("RemoveScript"),FunctionTemplate::New(RemoveScript)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteScriptFromPorts"),FunctionTemplate::New(DeleteScriptFromPorts)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAll"),FunctionTemplate::New(SaveAll)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(DeleteProfile)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPortSet"),FunctionTemplate::New(GetPortSet)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPortSetNames"),FunctionTemplate::New(GetPortSetNames)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydConfigBinding"), constructor);
}


Handle<Value> HoneydConfigBinding::New(const Arguments& args)
{
	HandleScope scope;

	HoneydConfigBinding* obj = new HoneydConfigBinding();
	obj->m_conf = HoneydConfiguration::Inst();
	obj->Wrap(args.This());

	return args.This();
}

Handle<Value> HoneydConfigBinding::GetPortSet(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 parameters")));
	}

	std::string profileName = cvv8::CastFromJS<string>(args[0]);
	std::string portSetName = cvv8::CastFromJS<string>(args[0]);

	Profile *profile = obj->m_conf->GetProfile(profileName);
	if(profile == NULL)
	{
		//ERROR
		return scope.Close( Null() );
	}

	return scope.Close( HoneydNodeJs::WrapPortSet( profile->GetPortSet(portSetName)) );
}

Handle<Value> HoneydConfigBinding::GetPortSetNames(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	std::string profileName = cvv8::CastFromJS<string>(args[0]);

	Profile *profile = obj->m_conf->GetProfile(profileName);
	if(profile == NULL)
	{
		//ERROR
		return scope.Close( Null() );
	}

	v8::Local<v8::Array> portArray = v8::Array::New();
	for(uint i = 0; i < profile->m_portSets.size(); i++)
	{
		portArray->Set(v8::Number::New(i),cvv8::CastToJS(profile->m_portSets[i]->m_name));
	}

	return scope.Close( portArray );
}

Handle<Value> HoneydConfigBinding::DeleteProfile(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	std::string profileToDelete = cvv8::CastFromJS<string>(args[0]);

	return scope.Close(Boolean::New(obj->m_conf->DeleteProfile(profileToDelete)));
}

Handle<Value> HoneydConfigBinding::AddNodes(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 4 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 4 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	string ipAddress = cvv8::CastFromJS<string>( args[1] );
	string interface = cvv8::CastFromJS<string>( args[2] );
	int count = cvv8::JSToInt32( args[3] );

	return scope.Close(Boolean::New(obj->m_conf->AddNodes(profile, ipAddress, interface, count)));
}


Handle<Value> HoneydConfigBinding::AddNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 4 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 4 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	string ipAddress = cvv8::CastFromJS<string>( args[1] );
	string mac = cvv8::CastFromJS<string>( args[2] );
	string interface = cvv8::CastFromJS<string>( args[3] );

	//TODO: Specify the PortSet here instead of NULL
	return scope.Close(Boolean::New(obj->m_conf->AddNode(profile,ipAddress,mac, interface, NULL)));
}

Handle<Value> HoneydConfigBinding::GetNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string MAC = cvv8::CastFromJS<string>(args[0]);

	Nova::Node *ret = obj->m_conf->GetNode(MAC);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapNode(ret));
	}
	else
	{
		return scope.Close( Null() );
	}
}

Handle<Value> HoneydConfigBinding::GetProfile(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);
	Nova::Profile *ret = obj->m_conf->GetProfile(name);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapProfile(ret));
	}
	else
	{
		return scope.Close( Null() );
	}

}

Handle<Value> HoneydConfigBinding::DeleteScriptFromPorts(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string profileName = cvv8::CastFromJS<string>(args[0]);

	obj->m_conf->DeleteScriptFromPorts(profileName);

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::AddScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() > 4 || args.Length() < 2)
	{
		// The service should be found dynamically, but unfortunately
		// within the nmap services file the services are linked to ports,
		// whereas our scripts have no such discrete affiliation.
		// I'm thinking of adding a searchable dropdown (akin to the
		// dojo select within the edit profiles page) that contains
		// all the services, and allow the user to associate the
		// uploaded script with one of these services.
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 to 4 parameters (3rd parameter is optional service, 4th parameter is optional osclass")));
	}

	Nova::Script script;
	script.m_name = cvv8::CastFromJS<string>(args[0]);
	script.m_path = cvv8::CastFromJS<string>(args[1]);

	if(args.Length() == 4)
	{
		script.m_service = cvv8::CastFromJS<string>(args[2]);
		script.m_osclass = cvv8::CastFromJS<string>(args[3]);
	}
	else
	{
		script.m_service = "";
		script.m_osclass = "";
	}

	if(obj->m_conf->AddScript(script))
	{
		return scope.Close(Boolean::New(true));
	}
	else
	{
		cout << "Script already present, doing nothing" << endl;
		return scope.Close(Boolean::New(false));
	}
}

Handle<Value> HoneydConfigBinding::RemoveScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	string scriptName = cvv8::CastFromJS<string>(args[0]);

	if(!obj->m_conf->DeleteScript(scriptName))
	{
		cout << "No registered script with name " << scriptName << endl;
		return scope.Close(Boolean::New(false));
	}

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::SaveAll(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	bool success = true;


	if (!obj->m_conf->WriteAllTemplatesToXML())
	{
		cout << "ERROR saving honeyd templates " << endl;
		success = false;
	}

	if (!obj->m_conf->WriteHoneydConfiguration()) {
		cout << "ERROR saving honeyd templates " << endl;
		success = false;
	}

	return scope.Close(Boolean::New(success));
}
