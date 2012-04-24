#ifndef __HONEYDNODEJS_H
#define __HONEYDNODEJS_H

#include <v8.h>
#include "v8Helper.h"
#include "NovaGuiTypes.h"
#include "FeatureSet.h"

class HoneydNodeJs
{

public:
    static v8::Handle<v8::Object> WrapNode(Nova::Node* node);

private:
    static v8::Persistent<v8::FunctionTemplate> m_NodeTemplate;

};


#endif // __SUSPECTJS_H
