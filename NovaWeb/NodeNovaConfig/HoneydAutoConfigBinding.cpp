#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <fstream>
#include "HoneydAutoConfigBinding.h"
#include "v8Helper.h"

using namespace v8;
using namespace std;

HoneydAutoConfigBinding::HoneydAutoConfigBinding()
{
};

HoneydAutoConfigBinding::~HoneydAutoConfigBinding()
{
};

void HoneydAutoConfigBinding::Init(Handle<Object> target) 
{
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("HoneydAutoConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  
  // Prototypes
  // tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUseAllInterfacesBinding"),FunctionTemplate::New(InvokeWrappedMethod<std::string, NovaConfigBinding, Config, &Config::GetUseAllInterfacesBinding>));
  // tpl->PrototypeTemplate()->Set(String::NewSymbol("AddIface"),FunctionTemplate::New(AddIface)->GetFunction());

  tpl->PrototypeTemplate()->Set(String::NewSymbol("RunAutoScan"),FunctionTemplate::New(RunAutoScan)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("HoneydAutoConfigBinding"), constructor);
}

Handle<Value> HoneydAutoConfigBinding::New(const Arguments& args)
{
  HandleScope scope;

  HoneydAutoConfigBinding* obj = new HoneydAutoConfigBinding();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> HoneydAutoConfigBinding::RunAutoScan(const Arguments& args)
{
  HandleScope scope;
  
  if(args.Length() < 1)
  {
    return ThrowException(Exception::TypeError(String::New("Must be invoked with at least one parameter")));
  }
  
  std::string numNodes = cvv8::CastFromJS<std::string>(args[0]);
  std::vector<std::string> additionalSubnets;
  
  for(int i = 1; i < args.Length(); i++)
  {
    additionalSubnets.push_back(cvv8::CastFromJS<std::string>(args[i]));
  }
  
  std::string systemCall = "honeydhostconfig " + numNodes + " ";
  
  for(uint i = 0; i < additionalSubnets.size() - 1; i++)
  {
    systemCall += additionalSubnets[i] + " ";
  }
  
  systemCall += additionalSubnets[additionalSubnets.size() - 1];
  
  //system(systemCall);
  
  //ifstream lockFile("/usr/share/nova/nova/hhconfig.lock");
  
  //while(lockFile.good()) {};
  
  std::cout << systemCall << std::endl;
  
  return args.This();
}

