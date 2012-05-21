#define BUILDING_NODE_EXTENSION

#include "CustomizeTraining.h"

using namespace std;
using namespace v8;
using namespace Nova;

CustomizeTrainingBinding::CustomizeTrainingBinding() {};
CustomizeTrainingBinding::~CustomizeTrainingBinding() {};

trainingSuspectMap * CustomizeTrainingBinding::GetChild()
{
	return m_map;
}

void CustomizeTrainingBinding::Init(v8::Handle<Object> target)
{
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("CustomizeTrainingBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetDescriptions"),FunctionTemplate::New(GetDescriptions)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUIDs"),FunctionTemplate::New(GetUIDs)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetHostile"),FunctionTemplate::New(GetHostile)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("SetIncluded"),FunctionTemplate::New(SetIncluded)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("Save"),FunctionTemplate::New(Save)->GetFunction());
  // Prototype

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("CustomizeTrainingBinding"), constructor);
}


v8::Handle<Value> CustomizeTrainingBinding::New(const Arguments& args)
{
  v8::HandleScope scope;

  CustomizeTrainingBinding *obj = new CustomizeTrainingBinding();
  
  obj->m_map = TrainingData::ParseTrainingDb("/usr/share/nova/nova/Config/training.db");
  
  if(obj->m_map == NULL)
  {
  	std::cout << "NULL point" << std::endl;
  }
  
  for(trainingSuspectMap::iterator header = obj->m_map->begin(); header != obj->m_map->end(); header++)
  {
    header->second->isIncluded = false;
  }
  
  obj->Wrap(args.This());

  return args.This();
}

v8::Handle<Value> CustomizeTrainingBinding::GetDescriptions(const Arguments& args)
{
	HandleScope scope;
	CustomizeTrainingBinding *obj = ObjectWrap::Unwrap<CustomizeTrainingBinding>(args.This());
	
	std::vector<std::string> descriptions;
	
	for(trainingSuspectMap::iterator header = obj->m_map->begin(); header != obj->m_map->end(); header++)
	{
		descriptions.push_back(header->second->description);
	}
	
	return scope.Close(cvv8::CastToJS(descriptions));
}

v8::Handle<Value> CustomizeTrainingBinding::GetUIDs(const Arguments& args)
{
	HandleScope scope;
	CustomizeTrainingBinding *obj = ObjectWrap::Unwrap<CustomizeTrainingBinding>(args.This());
	
	std::vector<std::string> uids;
	
	for(trainingSuspectMap::iterator header = obj->m_map->begin(); header != obj->m_map->end(); header++)
	{
		uids.push_back(header->second->uid);
	}
	
	return scope.Close(cvv8::CastToJS(uids));
}

v8::Handle<Value> CustomizeTrainingBinding::GetHostile(const Arguments& args)
{
	HandleScope scope;
	CustomizeTrainingBinding *obj = ObjectWrap::Unwrap<CustomizeTrainingBinding>(args.This());
	
	std::vector<bool> hostilities;
	
	for(trainingSuspectMap::iterator header = obj->m_map->begin(); header != obj->m_map->end(); header++)
	{
		hostilities.push_back(header->second->isHostile);
	}
	
	return scope.Close(cvv8::CastToJS(hostilities));
}

v8::Handle<Value> CustomizeTrainingBinding::SetIncluded(const Arguments& args)
{
	HandleScope scope;
	CustomizeTrainingBinding *obj = ObjectWrap::Unwrap<CustomizeTrainingBinding>(args.This());
	
	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 parameters")));
	}
	
	std::string uid = cvv8::CastFromJS<std::string>(args[0]);
	bool included = cvv8::CastFromJS<bool>(args[1]);
	bool success = false;
	
	if(obj->m_map->keyExists(uid))
	{
	  obj->m_map->get(uid)->isIncluded = included; 
		success = true;
	}
	else
	{
		std::cout << "Key [UID] not found in trainingSuspectMap" << std::endl; 
	}
	
	return scope.Close(cvv8::CastToJS(success));
}

v8::Handle<Value> CustomizeTrainingBinding::Save(const Arguments& args)
{
  HandleScope scope;
  CustomizeTrainingBinding *obj = ObjectWrap::Unwrap<CustomizeTrainingBinding>(args.This());
  
  std::string classification = TrainingData::MakaDataFile(*(obj->m_map));
  
  std::string path = Config::Inst()->GetPathHome() + "/" + Config::Inst()->GetPathTrainingFile();
  
  std::ofstream write(path.c_str());
  
  write << classification;
  
  write.close();
  
  return scope.Close(cvv8::CastToJS(false));
}
