#include "HoneydTypesJs.h"
#include "v8Helper.h"
#include "Logger.h"

#include <v8.h>
#include <string>
#include <node.h>

using namespace v8;
using namespace Nova;
using namespace std;

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
        proto->Set("GetName",       FunctionTemplate::New(InvokeMethod<std::string, Node, &Nova::Node::GetName>) );
        proto->Set("GetSubnet",     FunctionTemplate::New(InvokeMethod<std::string, Node, &Nova::Node::GetSubnet>) );
        proto->Set("GetInterface",  FunctionTemplate::New(InvokeMethod<std::string, Node, &Nova::Node::GetInterface>) );
        proto->Set("GetProfile",    FunctionTemplate::New(InvokeMethod<std::string, Node, &Nova::Node::GetProfile>) );
        proto->Set("GetIP",         FunctionTemplate::New(InvokeMethod<std::string, Node, &Nova::Node::GetIP>) );
        proto->Set("GetMAC",        FunctionTemplate::New(InvokeMethod<std::string, Node, &Nova::Node::GetMAC>) );
        proto->Set("IsEnabled",     FunctionTemplate::New(InvokeMethod<bool, Node, &Nova::Node::IsEnabled>) );
    }

    // Get the constructor from the template
    Handle<Function> ctor = m_NodeTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> nodePtr = External::New(node);
    result->SetInternalField(0,nodePtr);

    return scope.Close(result);
}


Handle<Object> HoneydNodeJs::WrapPort(port *port)
{
    HandleScope scope;  

    // Setup the template for the type if it hasn't been already
    if( m_portTemplate.IsEmpty() )
    {
        Handle<FunctionTemplate> nodeTemplate = FunctionTemplate::New();
        nodeTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        m_portTemplate = Persistent<FunctionTemplate>::New(nodeTemplate);

        // Javascript methods
        Local<Template> proto = m_portTemplate->PrototypeTemplate();
        proto->Set("GetPortName",    FunctionTemplate::New(InvokeMethod<std::string, Nova::port, &Nova::port::GetPortName>) );
        proto->Set("GetPortNum",     FunctionTemplate::New(InvokeMethod<std::string, Nova::port, &Nova::port::GetPortNum>) );
        proto->Set("GetType",        FunctionTemplate::New(InvokeMethod<std::string, Nova::port, &Nova::port::GetType>) );
        proto->Set("GetBehavior",    FunctionTemplate::New(InvokeMethod<std::string, Nova::port, &Nova::port::GetBehavior>) );
        proto->Set("GetScriptName",  FunctionTemplate::New(InvokeMethod<std::string, Nova::port, &Nova::port::GetScriptName>) );
        proto->Set("GetIsInherited",  FunctionTemplate::New(InvokeMethod<bool, Nova::port, &Nova::port::GetIsInherited>) );
    }

    // Get the constructor from the template
    Handle<Function> ctor = m_portTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> portPtr = External::New(port);
    result->SetInternalField(0,portPtr);

    return scope.Close(result);
}

Handle<Object> HoneydNodeJs::WrapProfile(profile *pfile)
{
    HandleScope scope;  
    // Setup the template for the type if it hasn't been already
    if( m_profileTemplate.IsEmpty() )
    {
        Handle<FunctionTemplate> nodeTemplate = FunctionTemplate::New();
        nodeTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        m_profileTemplate = Persistent<FunctionTemplate>::New(nodeTemplate);

        // Javascript methods
        Local<Template> proto = m_profileTemplate->PrototypeTemplate();
        proto->Set("GetName",           FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetName>) );
        proto->Set("GetTcpAction",      FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetTcpAction>) );
        proto->Set("GetUdpAction",      FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetUdpAction>) );
        proto->Set("GetIcmpAction",     FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetIcmpAction>) );
        proto->Set("GetPersonality",    FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetPersonality>) );
        proto->Set("GetEthernet",       FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetEthernet>) );
        proto->Set("GetUptimeMin",      FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetUptimeMin>) );
        proto->Set("GetUptimeMax",      FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetUptimeMax>) );
        proto->Set("GetDropRate",       FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetDropRate>) );
        proto->Set("GetParentProfile",  FunctionTemplate::New(InvokeMethod<std::string, profile, &Nova::profile::GetParentProfile>));
        
        proto->Set("isTcpActionInherited",  FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isTcpActionInherited>));
        proto->Set("isUdpActionInherited",  FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isUdpActionInherited>));
        proto->Set("isIcmpActionInherited", FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isIcmpActionInherited>));
        proto->Set("isPersonalityInherited",FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isPersonalityInherited>));
        proto->Set("isEthernetInherited",   FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isEthernetInherited>));
        proto->Set("isUptimeInherited",     FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isUptimeInherited>));
        proto->Set("isDropRateInherited",   FunctionTemplate::New(InvokeMethod<bool, profile, &Nova::profile::isDropRateInherited>));
    }

    // Get the constructor from the template
    Handle<Function> ctor = m_profileTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> profilePtr = External::New(pfile);
    result->SetInternalField(0,profilePtr);

    return scope.Close(result);
}

Persistent<FunctionTemplate> HoneydNodeJs::m_NodeTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::m_portTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::m_profileTemplate;
