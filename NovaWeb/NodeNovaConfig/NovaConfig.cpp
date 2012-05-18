#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "NovaConfigBinding.h"
#include "HoneydConfigBinding.h"
#include "VendorMacDbBinding.h"
#include "OsPersonalityDbBinding.h"
#include "HoneydProfileBinding.h"
#include "NovaNode.h"
#include "CustomizeTraining.h"

using namespace v8;

void InitAll(Handle<Object> target) {
  NovaNode::Init(target);
  NovaConfigBinding::Init(target);
  HoneydConfigBinding::Init(target);
  VendorMacDbBinding::Init(target);
  OsPersonalityDbBinding::Init(target);
  HoneydProfileBinding::Init(target);
  CustomizeTrainingBinding::Init(target);
}

NODE_MODULE(novaconfig, InitAll)
