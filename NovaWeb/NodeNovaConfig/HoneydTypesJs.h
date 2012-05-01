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
    static v8::Handle<v8::Object> WrapProfile(Nova::profile *profile);
    static v8::Handle<v8::Object> WrapPort(Nova::port *port);

private:
    // Helper functions
    // TODO: Use templates instead
    static v8::Handle<v8::Value> GetPortNames(const v8::Arguments& args);


    static v8::Persistent<v8::FunctionTemplate> m_NodeTemplate;
    static v8::Persistent<v8::FunctionTemplate> m_profileTemplate;
    static v8::Persistent<v8::FunctionTemplate> m_portTemplate;


};


#endif // __SUSPECTJS_H
