#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "NovaConfigBinding.h"

using namespace v8;

void InitAll(Handle<Object> target) {
  NovaConfigBinding::Init(target);
}

NODE_MODULE(novaconfig, InitAll)
