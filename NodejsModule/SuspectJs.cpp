#include "SuspectJs.h"
#include "Logger.h"
#include <boost/format.hpp>
#include <string>
#include <arpa/inet.h>

#include <iostream>

using namespace v8;
using boost::format;
using namespace Nova;

Handle<Object> SuspectJs::WrapSuspect(Suspect* suspect)
{
	HandleScope scope;
	// Setup the template for the type if it hasn't been already
	//
	if( m_SuspectTemplate.IsEmpty() )
	{
		Handle<FunctionTemplate> suspectTemplate = FunctionTemplate::New();
		suspectTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		m_SuspectTemplate = Persistent<FunctionTemplate>::New(suspectTemplate);

		// Javascript methods
		Local<Template> proto = m_SuspectTemplate->PrototypeTemplate();
		proto->Set("ToString",     		FunctionTemplate::New(InvokeMethod<string, Suspect, &Suspect::ToString>) );
		proto->Set("GetIdString",     		FunctionTemplate::New(InvokeMethod<string, Suspect, &Suspect::GetIdString>) );
		proto->Set("GetIpString",     		FunctionTemplate::New(InvokeMethod<string, Suspect, &Suspect::GetIpString>) );
		proto->Set("GetInterface",     		FunctionTemplate::New(InvokeMethod<string, Suspect, &Suspect::GetInterface>) );
		proto->Set("GetClassification", FunctionTemplate::New(InvokeMethod<double, Suspect, &Suspect::GetClassification>) );
		proto->Set("GetLastPacketTime", FunctionTemplate::New(InvokeMethod<long int, Suspect, &Suspect::GetLastPacketTime>) );
		proto->Set("GetIsHostile", 		FunctionTemplate::New(InvokeMethod<bool, Suspect, &Suspect::GetIsHostile>) );
		proto->Set("GetFlaggedByAlarm", FunctionTemplate::New(InvokeMethod<bool, Suspect, &Suspect::GetFlaggedByAlarm>) );
		
		
		proto->Set("GetRstCount", FunctionTemplate::New(InvokeMethod<uint64_t, Suspect, &Suspect::GetRstCount>) );
		proto->Set("GetAckCount", FunctionTemplate::New(InvokeMethod<uint64_t, Suspect, &Suspect::GetAckCount>) );
		proto->Set("GetSynCount", FunctionTemplate::New(InvokeMethod<uint64_t, Suspect, &Suspect::GetSynCount>) );
		proto->Set("GetFinCount", FunctionTemplate::New(InvokeMethod<uint64_t, Suspect, &Suspect::GetFinCount>) );
		proto->Set("GetSynAckCount", FunctionTemplate::New(InvokeMethod<uint64_t, Suspect, &Suspect::GetSynAckCount>) );

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
