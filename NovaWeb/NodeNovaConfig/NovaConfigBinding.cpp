#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "NovaConfigBinding.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

NovaConfigBinding::NovaConfigBinding() {};
NovaConfigBinding::~NovaConfigBinding() {};

void NovaConfigBinding::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("NovaConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ReadSetting"),FunctionTemplate::New(ReadSetting)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteSetting"),FunctionTemplate::New(WriteSetting)->GetFunction());


  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("NovaConfigBinding"), constructor);
}

Handle<Value> NovaConfigBinding::New(const Arguments& args) {
  HandleScope scope;

  NovaConfigBinding* obj = new NovaConfigBinding();
  obj->m_conf = Config::Inst();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> NovaConfigBinding::ReadSetting(const Arguments& args) 
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    std::string p1 = cvv8::CastFromJS<std::string>( args[0] );


	return scope.Close(String::New(obj->m_conf->ReadSetting(p1).c_str()));
}


Handle<Value> NovaConfigBinding::WriteSetting(const Arguments& args) 
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() != 2 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
    }

    std::string key = cvv8::CastFromJS<std::string>( args[0] );
    std::string value = cvv8::CastFromJS<std::string>( args[1] );


	return scope.Close(Boolean::New(obj->m_conf->WriteSetting(key, value)));
}

