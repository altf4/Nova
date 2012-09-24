#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include "HoneydConfiguration/HoneydConfiguration.h"
#include "HoneydAutoConfigBinding.h"
#include "v8Helper.h"

using namespace v8;
using namespace std;

HoneydAutoConfigBinding::HoneydAutoConfigBinding()
{
  m_hdconfig = NULL;
};

HoneydAutoConfigBinding::~HoneydAutoConfigBinding()
{
  if(m_hdconfig != NULL)
  {
    delete m_hdconfig;
  }
};

Nova::HoneydConfiguration* HoneydAutoConfigBinding::GetChild()
{
  return m_hdconfig;
}

void HoneydAutoConfigBinding::Init(Handle<Object> target) 
{
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("HoneydAutoConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  
  // Prototypes
  tpl->PrototypeTemplate()->Set(String::NewSymbol("RunAutoScan"),FunctionTemplate::New(RunAutoScan)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGeneratedNodeInfo"),FunctionTemplate::New(GetGeneratedNodeInfo)->GetFunction());

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
  
  if(args.Length() < 3)
  {
    return ThrowException(Exception::TypeError(String::New("Must be invoked with at exactly three parameters")));
  }
  
  std::string numNodes = cvv8::CastFromJS<std::string>(args[0]);
  std::string interfaces = cvv8::CastFromJS<std::string>(args[1]);
  std::string additionalSubnets = cvv8::CastFromJS<std::string>(args[2]);
  
  std::string systemCall = "haystackautoconfig -n " + numNodes;
  
  if(!cvv8::CastFromJS<std::string>(args[1]).empty())
  {
    systemCall += " -i " + interfaces;
  }
  
  if(!cvv8::CastFromJS<std::string>(args[2]).empty())
  {
    systemCall += " -a " + additionalSubnets;
  }
  
  std::cout << "Scanning and node generation commencing" << '\n';
  
  std::cout << "with command " << systemCall << '\n';
  
  system(systemCall.c_str());
 
  std::cout << "Autoconfig utility has finished running." << std::endl;
  
  return args.This();
}

Handle<Value> HoneydAutoConfigBinding::GetGeneratedNodeInfo(const Arguments& args)
{
  HandleScope scope;
  
  HoneydAutoConfigBinding* obj = ObjectWrap::Unwrap<HoneydAutoConfigBinding>(args.This());
  
  obj->m_hdconfig = new Nova::HoneydConfiguration();
  Nova::Config::Inst()->SetGroup("HaystackAutoConfig");
  
  obj->m_hdconfig->LoadAllTemplates(); 
  
  for(Nova::NodeTable::iterator it = obj->m_hdconfig->m_nodes.begin(); it != obj->m_hdconfig->m_nodes.end(); it++)
  {
    cout << it->first << ", " << it->second.m_pfile << '\n';
  }
  
  Handle<Value> ret = cvv8::CastToJS(obj->m_hdconfig->GeneratedProfilesStrings());
  
  return scope.Close(ret);
}




