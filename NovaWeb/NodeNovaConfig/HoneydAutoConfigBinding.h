#ifndef HHAUTOCONFIGBINDING_H
#define HHAUTOCONFIGBINDING_H

#include <node.h>
#include "HoneydConfiguration.h"
#include "Config.h"

class HoneydAutoConfigBinding : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
  
  Nova::HoneydConfiguration* GetChild();

private:
  HoneydAutoConfigBinding();
  ~HoneydAutoConfigBinding();
  
  Nova::HoneydConfiguration* m_hdconfig;

  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> RunAutoScan(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetGeneratedNodeInfo(const v8::Arguments& args);
};

#endif
