//============================================================================
// Name        : NovaNode.cpp
// Copyright   : DataSoft Corporation 2012
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
#include "ClassificationEngine.h"

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
		LOG(ERROR, "Error connecting to Novad","");
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
		//            LOG(DEBUG,"callback type " + cb.type,"");
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
			cout << "Got suspect clear on callback for " << cb.m_suspectIP << endl;
			s = new Suspect();
			s->SetIpAddress(cb.m_suspectIP);
			HandleSuspectCleared(s);

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
	if (m_suspects.count(suspect->GetIpAddress()) == 0)
	{
		delete m_suspects[suspect->GetIpAddress()];
	}	
	m_suspects[suspect->GetIpAddress()] = suspect;

	if( m_CallbackRegistered )
	{
		eio_req* req = (eio_req*) calloc(sizeof(*req),1);
		req->data = (void*) suspect;
		eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleNewSuspectOnV8Thread, suspect);
	}
}

void NovaNode::HandleSuspectCleared(Suspect *suspect)
{
	if (m_suspects.count(suspect->GetIpAddress()) == 0)
	{
		delete m_suspects[suspect->GetIpAddress()];
	}	

	if( m_SuspectClearedCallbackRegistered )
	{
		eio_req* req = (eio_req*) calloc(sizeof(*req),1);
		req->data = (void*) suspect;
		eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleSuspectClearedOnV8Thread, suspect);
	}
}

void NovaNode::HandleAllSuspectsCleared()
{
	for (map<in_addr_t, Suspect*>::iterator it = m_suspects.begin(); it != m_suspects.end(); it++)
	{
		delete ((*it).second);
	}
	m_suspects.clear();

	if (m_AllSuspectsClearedCallbackRegistered)
	{
		eio_req* req = (eio_req*) calloc(sizeof(*req),1);
		eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleAllClearedOnV8Thread, NULL);
	}
}

int NovaNode::HandleNewSuspectOnV8Thread(eio_req* req)
{
	Suspect* suspect = static_cast<Suspect*>(req->data);
	HandleScope scope;
	Local<Value> argv[1] = { Local<Value>::New(SuspectJs::WrapSuspect(suspect)) };
	m_CallbackFunction->Call(m_CallbackFunction, 1, argv);
	return 0;
}

int NovaNode::HandleSuspectClearedOnV8Thread(eio_req* req)
{
	Suspect* suspect = static_cast<Suspect*>(req->data);
	HandleScope scope;
	Local<Value> argv[1] = { Local<Value>::New(SuspectJs::WrapSuspect(suspect)) };
	m_SuspectClearedCallback->Call(m_SuspectClearedCallback, 1, argv);
	return 0;
}

int NovaNode::HandleAllClearedOnV8Thread(eio_req*)
{
	HandleScope scope;
	m_SuspectsClearedCallback->Call(m_SuspectsClearedCallback, 0, NULL);
	return 0;
}


void HandleCallbackError()
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

	NODE_SET_PROTOTYPE_METHOD(s_ct, "getSuspectList", getSuspectList);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearAllSuspects", ClearAllSuspects);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnNewSuspect", registerOnNewSuspect );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnAllSuspectsCleared", registerOnAllSuspectsCleared );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnSuspectCleared", registerOnSuspectCleared );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "CheckConnection", CheckConnection );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "CloseNovadConnection", (InvokeMethod<bool, Nova::CloseNovadConnection>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ConnectToNovad", (InvokeMethod<bool, Nova::ConnectToNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartNovad", (InvokeMethod<bool, bool, Nova::StartNovad>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopNovad", (InvokeMethod<bool, Nova::StopNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartHaystack", (InvokeMethod<bool, bool, Nova::StartHaystack>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopHaystack", (InvokeMethod<bool, Nova::StopHaystack>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsNovadUp", (InvokeMethod<bool, bool, Nova::IsNovadUp>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsHaystackUp", (InvokeMethod<bool, Nova::IsHaystackUp>) );
	//              
	//              NODE_SET_PROTOTYPE_METHOD(s_ct, "SaveAllSuspects", (InvokeMethod<Boolean, bool, Nova::SaveAllSuspects>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearSuspect", (InvokeMethod<bool, std::string, Nova::ClearSuspect>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ReclassifyAllSuspects", (InvokeMethod<bool, Nova::ReclassifyAllSuspects>) );
	
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetLocalIP", (InvokeMethod<std::string, std::string, Nova::GetLocalIP>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "Shutdown", Shutdown );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetSuspectDetailsString", GetSuspectDetailsString );


	// Javascript object constructor
	target->Set(String::NewSymbol("Instance"),
			s_ct->GetFunction());

	//LOG(DEBUG, "Attempting to connect to Novad...", "");
	//CheckConnection();
	InitializeUI();
	CheckInitNova();
	InitNovaCallbackProcessing();
	LOG(DEBUG, "Initialized NovaNode","");
}


Handle<Value> NovaNode::GetSuspectDetailsString(const Arguments &args) {
	HandleScope scope;

	string suspectIp = cvv8::CastFromJS<string>(args[0]);
	in_addr_t address;
	inet_pton(AF_INET, suspectIp.c_str(), &address);

	string details;
	Suspect *suspect = GetSuspectWithData(ntohl(address));
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

	return scope.Close(cvv8::CastToJS(ClassificationEngine::GetSupportedEngines()));
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

	Nova::CloseNovadConnection();
	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);
}

NovaNode::NovaNode() :
			m_count(0)
{
}

NovaNode::~NovaNode()
{

}

// Update our internal suspect list to that of novad
void NovaNode::SynchInternalList()
{
	vector<in_addr_t> *suspects;
	suspects = GetSuspectList(SUSPECTLIST_ALL);

	if (suspects == NULL)
	{
		cout << "Failed to get suspect list" << endl;
	}
	for (uint i = 0; i < suspects->size(); i++)
	{
		Suspect *suspect = GetSuspect(suspects->at(i));

		if (suspect != NULL)
		{
			HandleNewSuspect(suspect);
			if (m_suspects.count(suspect->GetIpAddress()) == 0)
			{
				delete m_suspects[suspect->GetIpAddress()];
			}	
			m_suspects[suspect->GetIpAddress()] = suspect;
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


Handle<Value> NovaNode::getSuspectList(const Arguments& args)
{
	HandleScope scope;

	LOG(DEBUG, "Triggered getSuspectList", "");

	if( ! args[0]->IsFunction() )
	{
		LOG(DEBUG, 
				"Attempted to register OnNewSuspect with non-function, excepting","");
		return ThrowException(Exception::TypeError(String::New("Argument must be a function")));
	}

	Local<Function> callbackFunction;
	callbackFunction = Local<Function>::New( args[0].As<Function>() );

	for (map<in_addr_t, Suspect*>::iterator it = m_suspects.begin(); it != m_suspects.end(); it++)
	{
		HandleNewSuspect((*it).second);
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
	LOG(DEBUG, "Registered callback for AllSuspectsCleared", "");
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
	LOG(DEBUG, "Registered callback for SuspectCleared", "");
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

map<in_addr_t, Suspect*> NovaNode::m_suspects = map<in_addr_t, Suspect*>();
bool NovaNode::m_CallbackRegistered=false;
bool NovaNode::m_AllSuspectsClearedCallbackRegistered=false;
bool NovaNode::m_SuspectClearedCallbackRegistered=false;
bool NovaNode::m_callbackRunning=false;
pthread_t NovaNode::m_NovaCallbackThread=0;





