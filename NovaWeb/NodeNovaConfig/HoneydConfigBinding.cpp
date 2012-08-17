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
	// Prototype

	// TODO: Ask ace about doing this the template way. The following segfaults,
	//tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(InvokeMethod<Boolean, bool, HoneydConfiguration, &HoneydConfiguration::LoadAllTemplates>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::LoadAllTemplates>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::SaveAllTemplates>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteHoneydConfiguration"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::WriteHoneydConfiguration>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGeneratedProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGeneratedProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNodeNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetNodeNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGroups"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGroups>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGeneratedNodeNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetGeneratedNodeNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSubnetNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetSubnetNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetScriptNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetScriptNames>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteProfile>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteNode"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteNode>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("CheckNotInheritingEmptyProfile"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::CheckNotInheritingEmptyProfile>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNodes"),FunctionTemplate::New(AddNewNodes)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNode"),FunctionTemplate::New(GetNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNode"),FunctionTemplate::New(AddNewNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfile"),FunctionTemplate::New(GetProfile)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPorts"),FunctionTemplate::New(GetPorts)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAll"),FunctionTemplate::New(SaveAll)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydConfigBinding"), constructor);
}


Handle<Value> HoneydConfigBinding::New(const Arguments& args)
{
	HandleScope scope;

	HoneydConfigBinding* obj = new HoneydConfigBinding();
	obj->m_conf = new HoneydConfiguration();
	obj->Wrap(args.This());

	return args.This();
}

Handle<Value> HoneydConfigBinding::AddPort(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 4 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 4 parameters")));
	}

	int portNum = cvv8::JSToInt16( args[0] );
	Nova::portProtocol isTCP = (Nova::portProtocol)cvv8::JSToInt32( args[1] );
	Nova::portBehavior portBehavior = (Nova::portBehavior)cvv8::JSToInt32( args[2] );
	string script = cvv8::CastFromJS<string>( args[3] );

	cout << "C++ Adding port " << portNum << " " << isTCP << " " << portBehavior << " " << script << endl;

	return scope.Close(String::New(obj->m_conf->AddPort(portNum, isTCP, portBehavior, script).c_str()));
}

Handle<Value> HoneydConfigBinding::AddNewNodes(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 5 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	string ipAddress = cvv8::CastFromJS<string>( args[1] );
	string interface = cvv8::CastFromJS<string>( args[2] );
	string subnet = cvv8::CastFromJS<string>( args[3] );
	int count = cvv8::JSToInt32( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->AddNewNodes(profile,ipAddress,interface,subnet,count)));
}


Handle<Value> HoneydConfigBinding::AddNewNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 5 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	string ipAddress = cvv8::CastFromJS<string>( args[1] );
	string mac = cvv8::CastFromJS<string>( args[2] );
	string interface = cvv8::CastFromJS<string>( args[3] );
	string subnet = cvv8::CastFromJS<string>( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->AddNewNode(profile,ipAddress,mac, interface,subnet)));
}

Handle<Value> HoneydConfigBinding::GetNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);

	Nova::Node *ret = obj->m_conf->GetNode(name);

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
	Nova::NodeProfile *ret = obj->m_conf->GetProfile(name);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapProfile(ret));
	}
	else
	{
		return scope.Close( Null() );
	}

}


Handle<Value> HoneydConfigBinding::GetPorts(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);

    vector<Nova::Port> ports = obj->m_conf->GetPorts(name);
    v8::Local<v8::Array> portArray = v8::Array::New();
    if(ports.empty() && name.compare("default"))
    {
      Port *nullSub = new Port();
      nullSub->m_portNum = "0";
      portArray->Set(v8::Number::New(0), HoneydNodeJs::WrapPort(nullSub));
      return scope.Close(portArray);
    }
    for (uint i = 0; i < ports.size(); i++) {
        Port *copy = new Port();
        *copy = ports.at(i);
        portArray->Set(v8::Number::New(i), HoneydNodeJs::WrapPort(copy));
    }

	return scope.Close(portArray);
}

Handle<Value> HoneydConfigBinding::SaveAll(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	bool success = true;


	if (!obj->m_conf->SaveAllTemplates())
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
