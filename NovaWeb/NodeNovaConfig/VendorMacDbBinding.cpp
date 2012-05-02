#define BUILDING_NODE_EXTENSION

#include "VendorMacDbBinding.h"

using namespace std;
using namespace v8;

VendorMacDbBinding::VendorMacDbBinding() {};
VendorMacDbBinding::~VendorMacDbBinding() {};

void VendorMacDbBinding::Init(v8::Handle<Object> target)
{
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("VendorMacDbBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetVendorNames"),FunctionTemplate::New(GetVendorNames)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("VendorMacDbBinding"), constructor);
}


v8::Handle<Value> VendorMacDbBinding::New(const Arguments& args)
{
  v8::HandleScope scope;

  VendorMacDbBinding* obj = new VendorMacDbBinding();
  obj->m_db = new VendorMacDb();
  obj->m_db->LoadPrefixFile();
  obj->Wrap(args.This());

  return args.This();
}


// Hacky work around helper functions
v8::Handle<Value> VendorMacDbBinding::GetVendorNames(const Arguments& args) 
{
	v8::HandleScope scope;
	VendorMacDbBinding* obj = ObjectWrap::Unwrap<VendorMacDbBinding>(args.This());

	v8::Handle<Value> names = cvv8::CastToJS(obj->m_db->GetVendorNames());

	return scope.Close(names);
}

