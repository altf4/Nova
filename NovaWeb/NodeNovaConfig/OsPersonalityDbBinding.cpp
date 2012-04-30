#define BUILDING_NODE_EXTENSION

#include "OsPersonalityDbBinding.h"

using namespace std;
using namespace v8;

OsPersonalityDbBinding::OsPersonalityDbBinding() {};

void OsPersonalityDbBinding::Init(v8::Handle<Object> target)
{
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("OsPersonalityDbBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPersonalityOptions"),FunctionTemplate::New(GetPersonalityOptions)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("OsPersonalityDbBinding"), constructor);
}


v8::Handle<Value> OsPersonalityDbBinding::New(const Arguments& args)
{
  v8::HandleScope scope;

  OsPersonalityDbBinding* obj = new OsPersonalityDbBinding();
  obj->m_db = new OsPersonalityDb();
  obj->m_db->LoadNmapPersonalitiesFromFile();
  obj->Wrap(args.This());

  return args.This();
}


// Hacky work around helper functions
v8::Handle<Value> OsPersonalityDbBinding::GetPersonalityOptions(const Arguments& args) 
{
	v8::HandleScope scope;
	OsPersonalityDbBinding* obj = ObjectWrap::Unwrap<OsPersonalityDbBinding>(args.This());

	v8::Handle<Value> names = cvv8::CastToJS(obj->m_db->GetPersonalityOptions());

	return scope.Close(names);
}

