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

void HoneydConfigBinding::Init(Handle<Object> target)
{
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("HoneydConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(LoadAllTemplates)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAllTemplates"),FunctionTemplate::New(SaveAllTemplates)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfileNames"),FunctionTemplate::New(GetProfileNames)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNodeNames"),FunctionTemplate::New(GetNodeNames)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSubnetNames"),FunctionTemplate::New(GetSubnetNames)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNewNodes"),FunctionTemplate::New(AddNewNodes)->GetFunction());
  
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNode"),FunctionTemplate::New(GetNode)->GetFunction());


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

Handle<Value> HoneydConfigBinding::LoadAllTemplates(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());
	return scope.Close(Boolean::New(obj->m_conf->LoadAllTemplates()));
}

Handle<Value> HoneydConfigBinding::SaveAllTemplates(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());
	obj->m_conf->SaveAllTemplates();
	return scope.Close(Boolean::New(true));
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

Handle<Value> HoneydConfigBinding::GetProfileNames(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

    std::string key = cvv8::CastFromJS<std::string>( args[0] );
    std::string value = cvv8::CastFromJS<std::string>( args[1] );

	Handle<Value> names = cvv8::CastToJS(obj->m_conf->GetProfileNames());

	return scope.Close(names);
}

Handle<Value> HoneydConfigBinding::GetNodeNames(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

    std::string key = cvv8::CastFromJS<std::string>( args[0] );
    std::string value = cvv8::CastFromJS<std::string>( args[1] );

	Handle<Value> names = cvv8::CastToJS(obj->m_conf->GetNodeNames());

	return scope.Close(names);
}


Handle<Value> HoneydConfigBinding::GetSubnetNames(const Arguments& args) 
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

    std::string key = cvv8::CastFromJS<std::string>( args[0] );
    std::string value = cvv8::CastFromJS<std::string>( args[1] );

	Handle<Value> names = cvv8::CastToJS(obj->m_conf->GetSubnetNames());

	return scope.Close(names);
}

