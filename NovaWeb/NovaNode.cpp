/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND. See the accompanying 
 * LICENSE file.
 */

#include <v8.h>
#include <node.h>

/* Nova headers */
#include "Connection.h"
#include "StatusQueries.h"
#include "NovadControl.h"
#include "CallbackHandler.h"
#include "Suspect.h"
#include "Logger.h"
#include <boost/format.hpp>

using namespace node;
using namespace v8;
using boost::format;
using namespace Nova;

class NovaNode;

class NovaNode: ObjectWrap
{
private:
	int m_count;
	static pthread_t m_NovaCallbackThread;
	static bool m_NovaCallbackHandlingContinue;
	static Persistent<Function> m_CallbackFunction;
	static bool m_CallbackRegistered;

	static void InitNovaCallbackProcessing()
	{
		m_CallbackRegistered = false;

        eio_custom(NovaCallbackHandling, EIO_PRI_DEFAULT, AfterNovaCallbackHandling, (void*)0);

	}

	static void CheckInitNova()
	{
        if( Nova::IsUp() )
        {
            return;
        }

        if( ! Nova::ConnectToNovad() )
        {
			LOG(ERROR, (format("File: %s Line: %d Error connecting to Novad")
                        %  __FILE__ %  __LINE__).str());
            return;
        }

        LOG(DEBUG, "CheckInitNova complete");
    }

	static void NovaCallbackHandling(eio_req* req)
	{
		using namespace Nova;
		CallbackChange cb;

        LOG(DEBUG, "Initializing Novad callback processing");

		do
		{
            cb = ProcessCallbackMessage();
            LOG(DEBUG,(format("callback type %d")%cb.type).str());
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
        LOG(DEBUG, "Novad hung up, closing callback processing");
	}

    static int AfterNovaCallbackHandling(eio_req* req)
    {
        return 0;
    }

	static void HandleNewSuspect(Suspect* suspect)
	{
        LOG(DEBUG, (format("File: %s Line: %d Novad informed us of new suspect")
                        %  __FILE__ %  __LINE__).str());

		if( m_CallbackRegistered )
		{
            eio_req* req = (eio_req*) calloc(sizeof(*req),1);
            req->data = (void*) suspect;
            eio_nop( EIO_PRI_DEFAULT, NovaNode::HandleNewSuspectOnV8Thread, suspect);
		}
	}

    static int HandleNewSuspectOnV8Thread(eio_req* req)
    {
        Suspect* suspect = static_cast<Suspect*>(req->data);
		HandleScope scope;
		LOG(DEBUG,"Invoking new suspect callback");
        m_CallbackFunction->Call(m_CallbackFunction, 0, NULL);
        return 0;
    }


	static void HandleCallbackError()
	{
        LOG(ERROR, (format("File: %s Line: %d Novad provided CALLBACK_ERROR, will continue and move on")
                        %  __FILE__ %  __LINE__).str());
	}

public:

	static Persistent<FunctionTemplate> s_ct;

	static void Init(Handle<Object> target)
	{
		HandleScope scope;

		Local<FunctionTemplate> t = FunctionTemplate::New(New);

		s_ct = Persistent<FunctionTemplate>::New(t);
		s_ct->InstanceTemplate()->SetInternalFieldCount(11);
		s_ct->SetClassName(String::NewSymbol("NovaNode"));

		// Javascript member methods
		NODE_SET_PROTOTYPE_METHOD(s_ct, "getSuspectList", getSuspectList);
		NODE_SET_PROTOTYPE_METHOD(s_ct, "IsUp", (InvokeMethod<Boolean, bool, Nova::IsUp>) );
		NODE_SET_PROTOTYPE_METHOD(s_ct, "OnNewSuspect", registerOnNewSuspect );

		/* NovadControl.h */
		NODE_SET_PROTOTYPE_METHOD(s_ct, "StartNovad", (InvokeMethod<Boolean, bool, Nova::StartNovad>) );
		NODE_SET_PROTOTYPE_METHOD(s_ct, "StopNovad", (InvokeMethod<Boolean, bool, Nova::StopNovad>) );
		NODE_SET_PROTOTYPE_METHOD(s_ct, "SaveAllSuspects", (InvokeMethod<Boolean, bool, Nova::SaveAllSuspects>) );
		NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearAllSuspects", (InvokeMethod<Boolean, bool, Nova::ClearAllSuspects>) );
		//NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearSuspect", (InvokeMethod<Boolean, bool, Nova::ClearSuspect>) )
		NODE_SET_PROTOTYPE_METHOD(s_ct, "ReclassifyAllSuspects", (InvokeMethod<Boolean, bool, Nova::ReclassifyAllSuspects>) );

		NODE_SET_PROTOTYPE_METHOD(s_ct, "Shutdown", Shutdown );

		// Javascript object constructor
		target->Set(String::NewSymbol("Instance"),
                    s_ct->GetFunction());
	    
		CheckInitNova();
		InitNovaCallbackProcessing();
        LOG(DEBUG, "Initialized NovaNode");
	}

	static Handle<Value> Shutdown(const Arguments& args)
	{
		HandleScope scope;
        LOG(DEBUG, (format("File: %s Line: %d Shutdown... closing Novad connection\n")
                        %  __FILE__ %  __LINE__).str());            
        Nova::CloseNovadConnection();
		Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
        return scope.Close(result);
    }

	NovaNode() :
		m_count(0)
	{
	}

	~NovaNode()
	{
	}

	static Handle<Value> New(const Arguments& args)
	{
		HandleScope scope;
		NovaNode* hw = new NovaNode();
		hw->Wrap(args.This());
		return args.This();
	}

	static Handle<Value> getSuspectList(const Arguments& args)
	{
		HandleScope scope;
		NovaNode* hw = ObjectWrap::Unwrap<NovaNode>(args.This());
		hw->m_count++;

		Handle<String> filterArgument;
		if( args.Length() == 1 )
		{
			filterArgument=args[0]->ToString();
		}
		else
		{
			filterArgument=String::New("");
		}

		// we don't yet have a method like this in the Nova core API.
		// so we're faking this out for now...
		Local<Array> result = Array::New(3);
		result->Set(Integer::New(0), String::New("suspect1"));
		result->Set(Integer::New(1), String::New("suspect2"));
		result->Set(Integer::New(2), String::New("suspect3"));

		return scope.Close(result);
	}

	static Handle<Value> registerOnNewSuspect(const Arguments& args)
	{
		HandleScope scope;

		if( ! args[0]->IsFunction() )
		{
            LOG(DEBUG, 
                (format("File: %s Line: %d Attempted to register OnNewSuspect with non-function, excepting")
                 % __FILE__ %  __LINE__).str()); 
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
    static void HandleOnNewSuspectWeakCollect(Persistent<Value> OnNewSuspectCallback, void* parameter)
    {
        // For now, we do nothing, meaning that the callback will always stay registered
        // and continue to be invoked
        // even if the original object upon which OnNewSuspect() was invoked has been
        // let go.
    }

	template <typename V8_RETURN, typename NATIVE_RETURN,  NATIVE_RETURN (*F)()> 
	static Handle<Value> InvokeMethod(const Arguments& args)
	{
		HandleScope scope;
		NovaNode* hw = ObjectWrap::Unwrap<NovaNode>(args.This());
		hw->m_count++;

		Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( F() ));
		return scope.Close(result);
	}   

};

Persistent<FunctionTemplate> NovaNode::s_ct;

Persistent<Function> NovaNode::m_CallbackFunction=Persistent<Function>();
bool NovaNode::m_CallbackRegistered=false;
pthread_t NovaNode::m_NovaCallbackThread=0;

extern "C" {
	static void init (Handle<Object> target)
	{
		NovaNode::Init(target);
	}

	NODE_MODULE(nova, init);


}



