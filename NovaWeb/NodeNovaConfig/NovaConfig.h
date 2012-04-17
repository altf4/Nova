#ifndef NovaConfig_H
#define NovaConfig_H

#include <node.h>
#include "Config.h"

class NovaConfig : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);

 private:
  NovaConfig();
  ~NovaConfig();

  Nova::Config *m_config;


  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetIsTraining(const v8::Arguments& args);


};

#endif
