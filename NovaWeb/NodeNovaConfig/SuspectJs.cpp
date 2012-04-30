#include "SuspectJs.h"
#include "Logger.h"
#include <boost/format.hpp>
#include <string>
#include <arpa/inet.h>

using namespace v8;
using boost::format;
using namespace Nova;

Handle<Object> SuspectJs::WrapSuspect(Suspect* suspect)
{
    HandleScope scope;  
    // Setup the template for the type if it hasn't been already
    if( m_SuspectTemplate.IsEmpty() )
    {
        Handle<FunctionTemplate> suspectTemplate = FunctionTemplate::New();
        suspectTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        m_SuspectTemplate = Persistent<FunctionTemplate>::New(suspectTemplate);

        // Javascript methods
        Local<Template> proto = m_SuspectTemplate->PrototypeTemplate();
        proto->Set("ToString",     FunctionTemplate::New(InvokeMethod<String, std::string, Suspect, &Suspect::ToString>) );
        proto->Set("GetInAddr", FunctionTemplate::New(InvokeMethod<String,  struct ::in_addr, Suspect, &Suspect::GetInAddr>) );
        proto->Set("GetClassification", FunctionTemplate::New(InvokeMethod<Number, double, Suspect, &Suspect::GetClassification>) );
        proto->Set("GetIsHostile", FunctionTemplate::New(InvokeMethod<Boolean, bool, Suspect, &Suspect::GetIsHostile>) );
        proto->Set("GetFlaggedByAlarm", FunctionTemplate::New(InvokeMethod<Boolean, bool, Suspect, &Suspect::GetFlaggedByAlarm>) );

        // Accessing features is a special case
        proto->Set("GetFeatures", FunctionTemplate::New(GetFeatures) );
    }

    // Get the constructor from the template
    Handle<Function> ctor = m_SuspectTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> suspectPtr = External::New(suspect);
    result->SetInternalField(0,suspectPtr);

    return scope.Close(result);
}


Persistent<FunctionTemplate> SuspectJs::m_SuspectTemplate;
