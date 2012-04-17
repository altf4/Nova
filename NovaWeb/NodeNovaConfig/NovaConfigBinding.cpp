#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "NovaConfigBinding.h"

using namespace v8;
using namespace Nova;

NovaConfigBinding::NovaConfigBinding() {};
NovaConfigBinding::~NovaConfigBinding() {};

void NovaConfigBinding::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("NovaConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetClassificationThreshold"),FunctionTemplate::New(GetClassificationThreshold)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetClassificationTimeout"),FunctionTemplate::New(GetClassificationTimeout)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetConfigFilePath"),FunctionTemplate::New(GetConfigFilePath)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetDataTTL"),FunctionTemplate::New(GetDataTTL)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetDoppelInterface"),FunctionTemplate::New(GetDoppelInterface)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetDoppelIp"),FunctionTemplate::New(GetDoppelIp)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetEnabledFeatures"),FunctionTemplate::New(GetEnabledFeatures)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetEnabledFeatureCount"),FunctionTemplate::New(GetEnabledFeatureCount)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetEps"),FunctionTemplate::New(GetEps)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGotoLive"),FunctionTemplate::New(GetGotoLive)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetInterface"),FunctionTemplate::New(GetInterface)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetIsDmEnabled"),FunctionTemplate::New(GetIsDmEnabled)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetIsTraining"),FunctionTemplate::New(GetIsTraining)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetK"),FunctionTemplate::New(GetK)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathCESaveFile"),FunctionTemplate::New(GetPathCESaveFile)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathConfigHoneydUser"),FunctionTemplate::New(GetPathConfigHoneydUser)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathConfigHoneydHS"),FunctionTemplate::New(GetPathConfigHoneydHS)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathPcapFile"),FunctionTemplate::New(GetPathPcapFile)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathTrainingCapFolder"),FunctionTemplate::New(GetPathTrainingCapFolder)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathTrainingFile"),FunctionTemplate::New(GetPathTrainingFile)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetReadPcap"),FunctionTemplate::New(GetReadPcap)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSaMaxAttempts"),FunctionTemplate::New(GetSaMaxAttempts)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSaPort"),FunctionTemplate::New(GetSaPort)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSaSleepDuration"),FunctionTemplate::New(GetSaSleepDuration)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSaveFreq"),FunctionTemplate::New(GetSaveFreq)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetTcpCheckFreq"),FunctionTemplate::New(GetTcpCheckFreq)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetTcpTimout"),FunctionTemplate::New(GetTcpTimout)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetThinningDistance"),FunctionTemplate::New(GetThinningDistance)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetKey"),FunctionTemplate::New(GetKey)->GetFunction());
//tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNeighbors"),FunctionTemplate::New(GetNeighbors)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGroup"),FunctionTemplate::New(GetGroup)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetLoggerPreferences"),FunctionTemplate::New(GetLoggerPreferences)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSMTPAddr"),FunctionTemplate::New(GetSMTPAddr)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSMTPDomain"),FunctionTemplate::New(GetSMTPDomain)->GetFunction());
//tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSMTPEmailRecipients"),FunctionTemplate::New(GetSMTPEmailRecipients)->GetFunction());
//tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSMTPPort"),FunctionTemplate::New(GetSMTPPort)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathBinaries"),FunctionTemplate::New(GetPathBinaries)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathWriteFolder"),FunctionTemplate::New(GetPathWriteFolder)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathReadFolder"),FunctionTemplate::New(GetPathReadFolder)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathIcon"),FunctionTemplate::New(GetPathIcon)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathHome"),FunctionTemplate::New(GetPathHome)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSqurtEnabledFeatures"),FunctionTemplate::New(GetSqurtEnabledFeatures)->GetFunction());
//tpl->PrototypeTemplate()->Set(String::NewSymbol("GetHaystackStorage"),FunctionTemplate::New(GetHaystackStorage)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUserPath"),FunctionTemplate::New(GetUserPath)->GetFunction());
tpl->PrototypeTemplate()->Set(String::NewSymbol(""),FunctionTemplate::New()->GetFunction());


  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("NovaConfigBinding"), constructor);
}

Handle<Value> NovaConfigBinding::New(const Arguments& args) {
  HandleScope scope;

  NovaConfigBinding* obj = new NovaConfigBinding();
  obj->counter_ = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
  obj->m_conf = Config::Inst();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> NovaConfigBinding::GetIsDmEnabled(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Boolean::New(obj->m_conf->GetIsDmEnabled()));
}

Handle<Value> NovaConfigBinding::GetClassificationThreshold(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetClassificationThreshold()));
}
Handle<Value> NovaConfigBinding::GetClassificationTimeout(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetClassificationTimeout()));
}
Handle<Value> NovaConfigBinding::GetConfigFilePath(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetConfigFilePath().c_str()));
}
Handle<Value> NovaConfigBinding::GetDataTTL(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetDataTTL()));
}
Handle<Value> NovaConfigBinding::GetDoppelInterface(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetDoppelInterface().c_str()));
}
Handle<Value> NovaConfigBinding::GetDoppelIp(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetDoppelIp().c_str()));
}
Handle<Value> NovaConfigBinding::GetEnabledFeatures(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetEnabledFeatures().c_str()));
}
Handle<Value> NovaConfigBinding::GetEnabledFeatureCount(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetEnabledFeatureCount()));
}
Handle<Value> NovaConfigBinding::GetEps(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetEps()));
}
Handle<Value> NovaConfigBinding::GetGotoLive(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Boolean::New(obj->m_conf->GetGotoLive()));
}
Handle<Value> NovaConfigBinding::GetInterface(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetInterface().c_str()));
}
Handle<Value> NovaConfigBinding::GetIsTraining(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Boolean::New(obj->m_conf->GetIsTraining()));
}
Handle<Value> NovaConfigBinding::GetK(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetK()));
}
Handle<Value> NovaConfigBinding::GetPathCESaveFile(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathCESaveFile().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathConfigHoneydUser(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathConfigHoneydUser().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathConfigHoneydHS(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathConfigHoneydHS().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathPcapFile(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathPcapFile().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathTrainingCapFolder(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathTrainingCapFolder().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathTrainingFile(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathTrainingFile().c_str()));
}
Handle<Value> NovaConfigBinding::GetReadPcap(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Boolean::New(obj->m_conf->GetReadPcap()));
}
Handle<Value> NovaConfigBinding::GetSaMaxAttempts(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetSaMaxAttempts()));
}
Handle<Value> NovaConfigBinding::GetSaPort(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetSaPort()));
}
Handle<Value> NovaConfigBinding::GetSaSleepDuration(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetSaSleepDuration()));
}
Handle<Value> NovaConfigBinding::GetSaveFreq(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetSaveFreq()));
}
Handle<Value> NovaConfigBinding::GetTcpCheckFreq(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetTcpCheckFreq()));
}
Handle<Value> NovaConfigBinding::GetTcpTimout(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetTcpTimout()));
}
Handle<Value> NovaConfigBinding::GetThinningDistance(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetThinningDistance()));
}
Handle<Value> NovaConfigBinding::GetKey(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetKey().c_str()));
}
//Handle<Value> NovaConfigBinding::GetNeighbors(const Arguments& args) {
//    HandleScope scope;
//    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
//    return scope.Close(Number::New(obj->m_conf->GetNeighbors()));
//}
Handle<Value> NovaConfigBinding::GetGroup(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetGroup().c_str()));
}
Handle<Value> NovaConfigBinding::GetLoggerPreferences(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetLoggerPreferences().c_str()));
}
Handle<Value> NovaConfigBinding::GetSMTPAddr(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetSMTPAddr().c_str()));
}
Handle<Value> NovaConfigBinding::GetSMTPDomain(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetSMTPDomain().c_str()));
}
//Handle<Value> NovaConfigBinding::GetSMTPEmailRecipients(const Arguments& args) {
 //   HandleScope scope;
 //   NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
 //   return scope.Close(String::New(obj->m_conf->GetSMTPEmailRecipients().c_str()));
//}
//Handle<Value> NovaConfigBinding::GetSMTPPort(const Arguments& args) {
//    HandleScope scope;
//    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
//    return scope.Close(Number::New(obj->m_conf->GetSMTPPort()));
//}
Handle<Value> NovaConfigBinding::GetPathBinaries(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathBinaries().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathWriteFolder(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathWriteFolder().c_str()));
}
Handle<Value> NovaConfigBinding::GetPathReadFolder(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathReadFolder().c_str()));
}

Handle<Value> NovaConfigBinding::GetPathIcon(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathIcon().c_str()));
}

Handle<Value> NovaConfigBinding::GetPathHome(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetPathHome().c_str()));
}

Handle<Value> NovaConfigBinding::GetSqurtEnabledFeatures(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(Number::New(obj->m_conf->GetSqurtEnabledFeatures()));
}

//Handle<Value> NovaConfigBinding::GetHaystackStorage(const Arguments& args) {
//    HandleScope scope;
//    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
//    return scope.Close(String::New(obj->m_conf->GetHaystackStorage()));
//}

Handle<Value> NovaConfigBinding::GetUserPath(const Arguments& args) {
    HandleScope scope;
    NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
    return scope.Close(String::New(obj->m_conf->GetUserPath().c_str()));
}
