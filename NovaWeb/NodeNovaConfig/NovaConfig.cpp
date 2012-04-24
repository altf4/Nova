#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "NovaConfigBinding.h"
#include "HoneydConfigBinding.h"

using namespace v8;

void InitAll(Handle<Object> target) {
  NovaConfigBinding::Init(target);
  HoneydConfigBinding::Init(target);
}

NODE_MODULE(novaconfig, InitAll)
