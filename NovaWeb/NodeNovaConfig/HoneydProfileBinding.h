#ifndef HONEYDPROFILEBINDING_H
#define HONEYDPROFILEBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "NovaGuiTypes.h"

class HoneydProfileBinding : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
  
  Nova::profile * GetChild();

 private:
  HoneydProfileBinding();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> Save(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddPort(const v8::Arguments& args);

  Nova::profile *m_pfile;
};

#endif
