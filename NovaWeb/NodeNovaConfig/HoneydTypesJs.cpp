#include "HoneydTypesJs.h"
#include "Logger.h"
#include <string>

using namespace v8;
using namespace Nova;

Handle<Object> HoneydNodeJs::WrapNode(Node* node)
{
    HandleScope scope;  
    // Setup the template for the type if it hasn't been already
    if( m_NodeTemplate.IsEmpty() )
    {
        Handle<FunctionTemplate> nodeTemplate = FunctionTemplate::New();
        nodeTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        m_NodeTemplate = Persistent<FunctionTemplate>::New(nodeTemplate);

        // Javascript methods
        Local<Template> proto = m_NodeTemplate->PrototypeTemplate();
        proto->Set("GetName",      	FunctionTemplate::New(InvokeMethod<String, std::string, Node, &Nova::Node::GetName>) );
		proto->Set("GetSubnet",     FunctionTemplate::New(InvokeMethod<String, std::string, Node, &Nova::Node::GetSubnet>) );
        proto->Set("GetInterface",  FunctionTemplate::New(InvokeMethod<String, std::string, Node, &Nova::Node::GetInterface>) );
        proto->Set("GetProfile",    FunctionTemplate::New(InvokeMethod<String, std::string, Node, &Nova::Node::GetProfile>) );
        proto->Set("GetIP",     	FunctionTemplate::New(InvokeMethod<String, std::string, Node, &Nova::Node::GetIP>) );
        proto->Set("GetMAC",     	FunctionTemplate::New(InvokeMethod<String, std::string, Node, &Nova::Node::GetMAC>) );
        proto->Set("IsEnabled",     FunctionTemplate::New(InvokeMethod<Boolean, bool, Node, &Nova::Node::IsEnabled>) );
    }

    // Get the template for the type
    Handle<FunctionTemplate> t = m_NodeTemplate;
    // Get the constructor from the template
    Handle<Function> ctor = m_NodeTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> nodePtr = External::New(node);
    result->SetInternalField(0,nodePtr);

    return scope.Close(result);
}


Persistent<FunctionTemplate> HoneydNodeJs::m_NodeTemplate;
