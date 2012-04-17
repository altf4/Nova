#include "ConfigJs.h"
#include "Logger.h"
#include <boost/format.hpp>
#include <string>
#include <arpa/inet.h>

using namespace v8;
using boost::format;
using namespace Nova;

Handle<Object> ConfigJs::WrapConfig(Config* config)
{
    HandleScope scope;  
    // Setup the template for the type if it hasn't been already
    if( m_ConfigTemplate.IsEmpty() )
    {
        Handle<FunctionTemplate> configTemplate = FunctionTemplate::New();
        configTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        m_ConfigTemplate = Persistent<FunctionTemplate>::New(configTemplate);

        // Javascript methods
        Local<Template> proto = m_ConfigTemplate->PrototypeTemplate();
        proto->Set("GetInterface", FunctionTemplate::New(InvokeMethod<String, std::string, Config, &Config::GetInterface>) );
        //proto->Set("GetClassification", FunctionTemplate::New(InvokeMethod<Number, double, Config, &Config::GetClassification>) );
        //proto->Set("GetIsHostile", FunctionTemplate::New(InvokeMethod<Boolean, bool, Config, &Config::GetIsHostile>) );
		
		target->Set(String::NewSymbol("Instance"),
				s_ct->GetFunction());
    }

    // Get the template for the type
    Handle<FunctionTemplate> t = m_ConfigTemplate;
    // Get the constructor from the template
    Handle<Function> ctor = m_ConfigTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> configPtr = External::New(config);
    result->SetInternalField(0,configPtr);

    return scope.Close(result);
}

Persistent<FunctionTemplate> ConfigJs::m_ConfigTemplate;
