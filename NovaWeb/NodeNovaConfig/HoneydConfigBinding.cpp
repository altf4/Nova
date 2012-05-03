#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "HoneydConfigBinding.h"
#include "HoneydTypesJs.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

HoneydConfigBinding::HoneydConfigBinding() {};
HoneydConfigBinding::~HoneydConfigBinding() {};

HoneydConfiguration * HoneydConfigBinding::GetChild() {
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


  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetProfileNames>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNodeNames"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetNodeNames>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSubnetNames"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetSubnetNames>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetScriptNames"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetScriptNames>));
  
  tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, std::string, &HoneydConfiguration::DeleteProfile>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteNode"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, std::string, &HoneydConfiguration::DeleteNode>));
 


  tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNodes"),FunctionTemplate::New(AddNewNodes)->GetFunction()); 
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNode"),FunctionTemplate::New(GetNode)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNode"),FunctionTemplate::New(AddNewNode)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfile"),FunctionTemplate::New(GetProfile)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPorts"),FunctionTemplate::New(GetPorts)->GetFunction());

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


Handle<Value> HoneydConfigBinding::AddNewNodes(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());
    
	if( args.Length() < 5 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    std::string profile = cvv8::CastFromJS<std::string>( args[0] );
    std::string ipAddress = cvv8::CastFromJS<std::string>( args[1] );
    std::string interface = cvv8::CastFromJS<std::string>( args[2] );
    std::string subnet = cvv8::CastFromJS<std::string>( args[3] );
    int count = cvv8::JSToInt32( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->AddNewNodes(profile,ipAddress,interface,subnet,count)));
}


Handle<Value> HoneydConfigBinding::AddNewNode(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());
    
	if( args.Length() < 5 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    std::string profile = cvv8::CastFromJS<std::string>( args[0] );
    std::string ipAddress = cvv8::CastFromJS<std::string>( args[1] );
    std::string mac = cvv8::CastFromJS<std::string>( args[2] );
    std::string interface = cvv8::CastFromJS<std::string>( args[3] );
    std::string subnet = cvv8::CastFromJS<std::string>( args[4] );

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

	std::string name = cvv8::CastFromJS<std::string>(args[0]);
	return scope.Close(HoneydNodeJs::WrapNode(obj->m_conf->GetNode(name)));
}

Handle<Value> HoneydConfigBinding::GetProfile(const Arguments& args)
{
  HandleScope scope;
  HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

  if (args.Length() != 1)
  {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
  }

  std::string name = cvv8::CastFromJS<std::string>(args[0]);
  return scope.Close(HoneydNodeJs::WrapProfile(obj->m_conf->GetProfile(name)));
}


Handle<Value> HoneydConfigBinding::GetPorts(const Arguments& args)
{
    HandleScope scope;
    HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

    if (args.Length() != 1)
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    std::string name = cvv8::CastFromJS<std::string>(args[0]);

    std::vector<Nova::port> ports = obj->m_conf->GetPorts(name);
    v8::Local<v8::Array> portArray = v8::Array::New();
    for (uint i = 0; i < ports.size(); i++) {
        port *copy = new port();
        *copy = ports.at(i);
        portArray->Set(v8::Number::New(i), HoneydNodeJs::WrapPort(copy));
    }

    return scope.Close(portArray);
}


