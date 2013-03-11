//============================================================================
// Name        : NovaNode.cpp
// Copyright   : DataSoft Corporation 2011-2013
//      Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Exposes Nova_UI_Core as a module for the node.js environment.
//============================================================================

#include "NovaNode.h"
#include "SuspectTable.h"
#include "Suspect.h"
#include "Config.h"

using namespace node;
using namespace v8;
using namespace Nova;
using namespace std;

void NovaNode::InitNovaCallbackProcessing()
{
	m_callbackRunning = false;

	eio_custom(NovaCallbackHandling, EIO_PRI_DEFAULT, AfterNovaCallbackHandling, (void*)0);

}

void NovaNode::CheckInitNova()
{
	if( Nova::IsNovadUp() )
	{
		return;
	}

	if( ! Nova::TryWaitConnectToNovad(3000) )
	{
		LOG(WARNING, "Could not connect to Novad. It is likely down.","");
		return;
	}

	SynchInternalList();

	LOG(DEBUG, "CheckInitNova complete","");
}

void NovaNode::NovaCallbackHandling(eio_req*)
{
	using namespace Nova;
	CallbackChange cb;

	LOG(DEBUG, "Initializing Novad callback processing","");

	CallbackHandler callbackHandler;

	m_callbackRunning = true;
	do
	{
		Suspect *s;
		cb = callbackHandler.ProcessCallbackMessage();
		switch( cb.m_type )
		{
		case CALLBACK_NEW_SUSPECT:
			HandleNewSuspect(cb.m_suspect);
			break;

		case CALLBACK_ERROR:
			HandleCallbackError();
			break;

		case CALLBACK_ALL_SUSPECTS_CLEARED:
			HandleAllSuspectsCleared();
			break;

		case CALLBACK_SUSPECT_CLEARED:
			s = new Suspect();
			s->SetIdentifier(cb.m_suspectIP);
			LOG(DEBUG, "Got a clear callback request for a suspect on interface " + cb.m_suspectIP.m_ifname(), "");
			HandleSuspectCleared(s);
			break;

		default:
			break;
		}
	}
	while(cb.m_type != CALLBACK_HUNG_UP);         
	LOG(DEBUG, "Novad hung up, closing callback processing","");
	m_callbackRunning = false;
}

int NovaNode::AfterNovaCallbackHandling(eio_req*)
{
	return 0;
}

void NovaNode::HandleNewSuspect(Suspect* suspect)
{
	eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleNewSuspectOnV8Thread, suspect);
}


// Sends a copy of the suspect from the C++ side to the server side Javascript side
// Note: Only call this from inside the v8 thread. Provides no locking.
void NovaNode::SendSuspect(Suspect* suspect)
{
	// Do we have anyplace to actually send the suspect?
	if (m_CallbackRegistered) {
		// Make a copy of it so we can safely delete the one in the suspect table when updates come in
		Suspect *suspectCopy = new Suspect((*suspect));

		// Wrap it in Javascript voodoo so main.js can handle it
		v8::Persistent<Value> weak_handle = Persistent<Value>::New(SuspectJs::WrapSuspect(suspectCopy));

		// When JS is done with it, call DoneWithSuspectCallback to clean it up
		weak_handle.MakeWeak(suspectCopy, &DoneWithSuspectCallback);

		// Send it off to the callback in main.js
		m_CallbackFunction->Call(m_CallbackFunction, 1, &weak_handle);
	}
}

void NovaNode::HandleSuspectCleared(Suspect *suspect)
{
	eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleSuspectClearedOnV8Thread, suspect);
}

void NovaNode::HandleAllSuspectsCleared()
{
	eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleAllClearedOnV8Thread, NULL);
}

int NovaNode::HandleNewSuspectOnV8Thread(eio_req* req)
{
	HandleScope scope;
	
	Suspect* suspect = static_cast<Suspect*>(req->data);
		
	if (suspect != NULL) {	
		if (m_suspects.keyExists(suspect->GetIdentifier())) {
			delete m_suspects[suspect->GetIdentifier()];
		}
		
		m_suspects[suspect->GetIdentifier()] = suspect;
		SendSuspect(suspect);
	} else {
		LOG(DEBUG, "HandleNewSuspectOnV8Thread got a NULL suspect pointer. Ignoring.", "");
	}
	return 0;

}

void NovaNode::DoneWithSuspectCallback(Persistent<Value> suspect, void *paramater) {
	Suspect *s = static_cast<Suspect*>(paramater);
	delete s;	
	suspect.ClearWeak();
	suspect.Dispose();
	suspect.Clear();
}

int NovaNode::HandleSuspectClearedOnV8Thread(eio_req* req)
{
	HandleScope scope;
	Suspect* suspect = static_cast<Suspect*>(req->data);
	if (m_suspects.keyExists(suspect->GetIdentifier())) {
		delete m_suspects[suspect->GetIdentifier()];
	}
	m_suspects.erase(suspect->GetIdentifier());

	v8::Persistent<Value> weak_handle = Persistent<Value>::New(SuspectJs::WrapSuspect(suspect));
	weak_handle.MakeWeak(suspect, &DoneWithSuspectCallback);
	Persistent<Value> argv[1] = { weak_handle };	
	m_SuspectClearedCallback->Call(m_SuspectClearedCallback, 1, argv);
	
	return 0;
}

int NovaNode::HandleAllClearedOnV8Thread(eio_req*)
{
	HandleScope scope;
	for(SuspectHashTable::iterator it = m_suspects.begin(); it != m_suspects.end(); it++)
	{
		delete (*it).second;
	}
	m_suspects.clear();
	m_SuspectsClearedCallback->Call(m_SuspectsClearedCallback, 0, NULL);
	return 0;
}


void NovaNode::HandleCallbackError()
{
	LOG(ERROR, "Novad provided CALLBACK_ERROR, will continue and move on","");
}

void NovaNode::Init(Handle<Object> target)
{
	HandleScope scope;

	Local<FunctionTemplate> t = FunctionTemplate::New(New);

	s_ct = Persistent<FunctionTemplate>::New(t);
	s_ct->InstanceTemplate()->SetInternalFieldCount(1);
	s_ct->SetClassName(String::NewSymbol("NovaNode"));

	// Javascript member methods
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetFeatureNames", GetFeatureNames);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetDIM", GetDIM);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetSupportedEngines", GetSupportedEngines);

	NODE_SET_PROTOTYPE_METHOD(s_ct, "sendSuspectList", sendSuspectList);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "sendSuspect", sendSuspect);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearAllSuspects", ClearAllSuspects);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnNewSuspect", registerOnNewSuspect );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnAllSuspectsCleared", registerOnAllSuspectsCleared );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnSuspectCleared", registerOnSuspectCleared );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "CheckConnection", CheckConnection );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "CloseNovadConnection", (InvokeMethod<bool, Nova::DisconnectFromNovad>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ConnectToNovad", (InvokeMethod<bool, Nova::ConnectToNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartNovad", (InvokeMethod<bool, bool, Nova::StartNovad>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopNovad", (InvokeMethod<bool, Nova::StopNovad>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "HardStopNovad", (InvokeMethod<bool, Nova::HardStopNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartHaystack", (InvokeMethod<bool, bool, Nova::StartHaystack>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopHaystack", (InvokeMethod<bool, Nova::StopHaystack>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsNovadUp", (InvokeMethod<bool, bool, Nova::IsNovadUp>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsHaystackUp", (InvokeMethod<bool, Nova::IsHaystackUp>) );
	//              
	//              NODE_SET_PROTOTYPE_METHOD(s_ct, "SaveAllSuspects", (InvokeMethod<Boolean, bool, Nova::SaveAllSuspects>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ReclassifyAllSuspects", (InvokeMethod<bool, Nova::ReclassifyAllSuspects>) );
	
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetLocalIP", (InvokeMethod<std::string, std::string, Nova::GetLocalIP>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetSubnetFromInterface", (InvokeMethod<std::string, std::string, Nova::GetSubnetFromInterface>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "Shutdown", Shutdown );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearSuspect", ClearSuspect );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetSuspectDetailsString", GetSuspectDetailsString );

	// Javascript object constructor
	target->Set(String::NewSymbol("Instance"),
			s_ct->GetFunction());
}


Handle<Value> NovaNode::GetSuspectDetailsString(const Arguments &args) {
	HandleScope scope;
	string details;

	string suspectIp = cvv8::CastFromJS<string>(args[0]);
	string suspectInterface = cvv8::CastFromJS<string>(args[1]);

	struct in_addr address;
	inet_pton(AF_INET, suspectIp.c_str(), &address);

	SuspectID_pb id;
	id.set_m_ip(htonl(address.s_addr));
	id.set_m_ifname(suspectInterface);

	Suspect *suspect = GetSuspectWithData(id);
	if (suspect != NULL) {
		details = suspect->ToString();
		details += "\n";
		details += suspect->GetFeatureSet().toString();
		delete suspect;
	} else {
		details = "Unable to complete request";
	}

	return scope.Close(cvv8::CastToJS(details));
}

// Figure out what the names of features are in the featureset
Handle<Value> NovaNode::GetFeatureNames(const Arguments &)
{
	HandleScope scope;

	vector<string> featureNames;
	for (int i = 0; i < DIM; i++)
	{
		featureNames.push_back(Nova::FeatureSet::m_featureNames[i]);
	}

	return scope.Close(cvv8::CastToJS(featureNames));
}

Handle<Value> NovaNode::ClearAllSuspects(const Arguments &)
{
	HandleScope scope;
	
	HandleAllSuspectsCleared();
	
	bool cleared = Nova::ClearAllSuspects();

	Local<Boolean> result = Local<Boolean>::New( Boolean::New(cleared) );
	return scope.Close(result);
}

Handle<Value> NovaNode::GetSupportedEngines(const Arguments &)
{
	HandleScope scope;

	return scope.Close(cvv8::CastToJS(Nova::Config::Inst()->GetSupportedEngines()));
}

Handle<Value> NovaNode::GetDIM(const Arguments &)
{
	HandleScope scope;

	return scope.Close(cvv8::CastToJS(DIM));
}

// Checks if we lost the connection. If so, tries to reconnect
Handle<Value> NovaNode::CheckConnection(const Arguments &)
{
	HandleScope scope;

	if (!m_callbackRunning)
	{
		LOG(DEBUG, "Attempting to connect to Novad...", "");
		CheckInitNova();
		InitNovaCallbackProcessing();
	}

	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);
}


Handle<Value> NovaNode::Shutdown(const Arguments &)
{
	HandleScope scope;
	LOG(DEBUG, "Shutdown... closing Novad connection","");

	Nova::DisconnectFromNovad();
	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);
}

Handle<Value> NovaNode::ClearSuspect(const Arguments &args)
{
	HandleScope scope;
	string suspectIp = cvv8::CastFromJS<string>(args[0]);
	string suspectInterface = cvv8::CastFromJS<string>(args[1]);

	in_addr_t address;
	inet_pton(AF_INET, suspectIp.c_str(), &address);

	SuspectID_pb id;
	id.set_m_ifname(suspectInterface);
	id.set_m_ip(ntohl(address));
	return scope.Close(Boolean::New(Nova::ClearSuspect(id)));
}

NovaNode::NovaNode() :
			m_count(0)
{
}

NovaNode::~NovaNode()
{
	cout << "Destructing NovaNode." << endl;
	for(SuspectHashTable::iterator it = m_suspects.begin(); it != m_suspects.end(); it++)
	{
		delete (*it).second;
	}
	m_suspects.clear();
}

// Update our internal suspect list to that of novad
void NovaNode::SynchInternalList()
{
	vector<SuspectID_pb> suspects = GetSuspectList(SUSPECTLIST_ALL);

	for (uint i = 0; i < suspects.size(); i++)
	{
		Suspect *suspect = GetSuspect(suspects.at(i));

		if (suspect != NULL)
		{
			HandleNewSuspect(suspect);
		}
		else
		{
			cout << "Error: No suspect received" << endl;
		}
	}
}


Handle<Value> NovaNode::New(const Arguments& args)
{
	HandleScope scope;
	NovaNode* hw = new NovaNode();
	hw->Wrap(args.This());
	return args.This();
}


Handle<Value> NovaNode::sendSuspect(const Arguments& args)
{
	HandleScope scope;

	string suspectIp = cvv8::CastFromJS<string>(args[0]);
	string suspectInterface = cvv8::CastFromJS<string>(args[1]);
	Local<Function> callbackFunction = Local<Function>::Cast(args[2]);

	struct in_addr address;
	inet_pton(AF_INET, suspectIp.c_str(), &address);

	SuspectID_pb id;
	id.set_m_ifname(suspectInterface);
	id.set_m_ip(htonl(address.s_addr));

	Suspect *suspect = GetSuspect(id);

	if (suspect == NULL) {
		Local<Boolean> result = Local<Boolean>::New( Boolean::New(false) );
		return scope.Close(result);
	}
	

	v8::Persistent<Value> weak_handle = Persistent<Value>::New(SuspectJs::WrapSuspect(suspect));
	weak_handle.MakeWeak(suspect, &DoneWithSuspectCallback);
	return scope.Close(weak_handle);
}


Handle<Value> NovaNode::sendSuspectList(const Arguments& args)
{
	HandleScope scope;

	LOG(DEBUG, "Triggered sendSuspectList", "");

	if( ! args[0]->IsFunction() )
	{
		LOG(DEBUG, 
				"Attempted to register OnNewSuspect with non-function, excepting","");
		return ThrowException(Exception::TypeError(String::New("Argument must be a function")));
	}

	Local<Function> callbackFunction;
	callbackFunction = Local<Function>::New( args[0].As<Function>() );

	    
	if (m_CallbackRegistered) {
		for(SuspectHashTable::iterator it = m_suspects.begin(); it != m_suspects.end(); it++)
		{
			SendSuspect((*it).second);
		}
	}

	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);
}

Handle<Value> NovaNode::registerOnNewSuspect(const Arguments& args)
{
	HandleScope scope;

	if( ! args[0]->IsFunction() )
	{
		LOG(DEBUG, 
				"Attempted to register OnNewSuspect with non-function, excepting","");
		return ThrowException(Exception::TypeError(String::New("Argument must be a function")));
	}

	m_CallbackFunction = Persistent<Function>::New( args[0].As<Function>() );
	m_CallbackFunction.MakeWeak(0, HandleOnNewSuspectWeakCollect);


	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	m_CallbackRegistered = true;
	return scope.Close(result);      
}

Handle<Value> NovaNode::registerOnAllSuspectsCleared(const Arguments& args)
{
	HandleScope scope;

	if( ! args[0]->IsFunction() )
	{
		LOG(DEBUG, 
				"Attempted to register OnAllSuspectsCleared with non-function, excepting","");
		return ThrowException(Exception::TypeError(String::New("Argument must be a function")));
	}

	m_SuspectsClearedCallback = Persistent<Function>::New( args[0].As<Function>() );
	m_SuspectsClearedCallback.MakeWeak(0, HandleOnNewSuspectWeakCollect);

	m_AllSuspectsClearedCallbackRegistered = true;
	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);      
}

Handle<Value> NovaNode::registerOnSuspectCleared(const Arguments& args)
{
	HandleScope scope;

	if( ! args[0]->IsFunction() )
	{
		LOG(DEBUG, 
				"Attempted to register OnSuspectCleared with non-function, excepting","");
		return ThrowException(Exception::TypeError(String::New("Argument must be a function")));
	}

	m_SuspectClearedCallback = Persistent<Function>::New( args[0].As<Function>() );
	m_SuspectClearedCallback.MakeWeak(0, HandleOnNewSuspectWeakCollect);

	m_SuspectClearedCallbackRegistered = true;
	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);      
}

// Invoked when the only one referring to an OnNewSuspect handler is us, i.e. no JS objects
// are holding onto it.  So it's up to us to decide what to do about it.
void NovaNode::HandleOnNewSuspectWeakCollect(Persistent<Value> , void *)
{
	// For now, we do nothing, meaning that the callback will always stay registered
	// and continue to be invoked
	// even if the original object upon which OnNewSuspect() was invoked has been
	// let go.
}

Persistent<FunctionTemplate> NovaNode::s_ct;

Persistent<Function> NovaNode::m_CallbackFunction=Persistent<Function>();
Persistent<Function> NovaNode::m_SuspectsClearedCallback=Persistent<Function>();
Persistent<Function> NovaNode::m_SuspectClearedCallback=Persistent<Function>();

SuspectHashTable NovaNode::m_suspects = SuspectHashTable();
//map<SuspectIdentifier, Suspect*> NovaNode::m_suspects = map<SuspectIdentifier, Suspect*>();
bool NovaNode::m_CallbackRegistered=false;
bool NovaNode::m_AllSuspectsClearedCallbackRegistered=false;
bool NovaNode::m_SuspectClearedCallbackRegistered=false;
bool NovaNode::m_callbackRunning=false;
pthread_t NovaNode::m_NovaCallbackThread=0;

