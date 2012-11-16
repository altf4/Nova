#ifndef __HONEYDNODEJS_H
#define __HONEYDNODEJS_H

#include <v8.h>
#include "v8Helper.h"
#include "FeatureSet.h"
#include "HoneydConfiguration/Node.h"
#include "HoneydConfiguration/Profile.h"
#include "HoneydConfiguration/Port.h"

class HoneydNodeJs
{

public:
    static v8::Handle<v8::Object> WrapNode(Nova::Node* node);
    //static v8::Handle<v8::Object> WrapProfile(Nova::Profile *profile);
    //static v8::Handle<v8::Object> WrapPort(Nova::Port *port);

private:

	static v8::Persistent<v8::FunctionTemplate> m_NodeTemplate;
	static v8::Persistent<v8::FunctionTemplate> m_profileTemplate;
	static v8::Persistent<v8::FunctionTemplate> m_portTemplate;
};


#endif // __SUSPECTJS_H
