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

void NovaNode::NovaCallbackHandling(eio_req __attribute__((__unused__)) *req)
{
	using namespace Nova;
	CallbackChange cb;

	LOG(DEBUG, "Initializing Novad callback processing","");

	m_callbackRunning = true;
	do
	{
		cb = ProcessCallbackMessage();
		//            LOG(DEBUG,"callback type " + cb.type,"");
		switch( cb.type )
		{
			case CALLBACK_NEW_SUSPECT:
				HandleNewSuspect(cb.suspect);
				break;

			case CALLBACK_ERROR:
				HandleCallbackError();
				break;

			default:
				break;
		}
	}
	while(cb.type != CALLBACK_HUNG_UP);         
	LOG(DEBUG, "Novad hung up, closing callback processing","");
	m_callbackRunning = false;
}

int NovaNode::AfterNovaCallbackHandling(eio_req __attribute__((__unused__)) *req)
{
	return 0;
}

void NovaNode::HandleNewSuspect(Suspect* suspect)
{
	LOG(DEBUG, "Novad informed us of new suspect", "");

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

int NovaNode::HandleNewSuspectOnV8Thread(eio_req* req)
{
	Suspect* suspect = static_cast<Suspect*>(req->data);
	HandleScope scope;
	LOG(DEBUG,"Invoking new suspect callback","");
	Local<Value> argv[1] = { Local<Value>::New(SuspectJs::WrapSuspect(suspect)) };
	m_CallbackFunction->Call(m_CallbackFunction, 1, argv);
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
	NODE_SET_PROTOTYPE_METHOD(s_ct, "getSuspectList", getSuspectList);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "registerOnNewSuspect", registerOnNewSuspect );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "CheckConnection", CheckConnection );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "CloseNovadConnection", (InvokeMethod<bool, Nova::CloseNovadConnection>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ConnectToNovad", (InvokeMethod<bool, Nova::ConnectToNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartNovad", (InvokeMethod<bool, Nova::StartNovad>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopNovad", (InvokeMethod<bool, Nova::StopNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartHaystack", (InvokeMethod<bool, Nova::StartHaystack>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopHaystack", (InvokeMethod<bool, Nova::StopHaystack>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsNovadUp", (InvokeMethod<bool, Boolean, bool, Nova::IsNovadUp>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsHaystackUp", (InvokeMethod<bool, Nova::IsHaystackUp>) );
	//              
	//              NODE_SET_PROTOTYPE_METHOD(s_ct, "SaveAllSuspects", (InvokeMethod<Boolean, bool, Nova::SaveAllSuspects>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearAllSuspects", (InvokeMethod<bool, Nova::ClearAllSuspects>) );
	//NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearSuspect", (InvokeMethod<bool, Nova::ClearSuspect>) )
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ReclassifyAllSuspects", (InvokeMethod<bool, Nova::ReclassifyAllSuspects>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "Shutdown", Shutdown );

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

// Checks if we lost the connection. If so, tries to reconnect
Handle<Value> NovaNode::CheckConnection(const Arguments __attribute__((__unused__)) & args)
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


Handle<Value> NovaNode::Shutdown(const Arguments __attribute__((__unused__)) & args)
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

// Invoked when the only one referring to an OnNewSuspect handler is us, i.e. no JS objects
// are holding onto it.  So it's up to us to decide what to do about it.
void NovaNode::HandleOnNewSuspectWeakCollect(Persistent<Value> __attribute__((__unused__)) OnNewSuspectCallback, void __attribute__((__unused__)) * parameter)
{
	// For now, we do nothing, meaning that the callback will always stay registered
	// and continue to be invoked
	// even if the original object upon which OnNewSuspect() was invoked has been
	// let go.
}

Persistent<FunctionTemplate> NovaNode::s_ct;

Persistent<Function> NovaNode::m_CallbackFunction=Persistent<Function>();
std::map<in_addr_t, Suspect*> NovaNode::m_suspects = map<in_addr_t, Suspect*>();
bool NovaNode::m_CallbackRegistered=false;
bool NovaNode::m_callbackRunning=false;
pthread_t NovaNode::m_NovaCallbackThread=0;





