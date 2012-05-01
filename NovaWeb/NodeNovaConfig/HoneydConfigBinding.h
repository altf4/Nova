#ifndef HONEYDCONFIGBINDING_H
#define HONEYDCONFIGBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "HoneydConfiguration.h"

class HoneydConfigBinding : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);

 private:
  HoneydConfigBinding();
  ~HoneydConfigBinding();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);

  static v8::Handle<v8::Value> SaveAllTemplates(const v8::Arguments& args);
  static v8::Handle<v8::Value> LoadAllTemplates(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetProfileNames(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetNodeNames(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetSubnetNames(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddNewNodes(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddNewNode(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> GetNode(const v8::Arguments& args);
  static v8::Handle<v8::Value> DeleteNode(const v8::Arguments& args);
  static v8::Handle<v8::Value> DeleteProfile(const v8::Arguments& args);

  static v8::Handle<v8::Value> GetProfile(const v8::Arguments& args);

  
  Nova::HoneydConfiguration *m_conf;
};

#endif
