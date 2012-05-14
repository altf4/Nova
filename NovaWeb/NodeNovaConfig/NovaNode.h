//============================================================================
// Name        : NovaNode.h
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

#include <v8.h>
#include <node.h>

#include <map>

/* Nova headers */
#include "nova_ui_core.h"
#include "Suspect.h"
#include "Logger.h"

/* NovaNode headers */
#include "SuspectJs.h"
#include "v8Helper.h"

using namespace node;
using namespace v8;
using namespace Nova;
using namespace std;

class NovaNode: ObjectWrap
{
	private:
		int m_count;
		static pthread_t m_NovaCallbackThread;
		static bool m_NovaCallbackHandlingContinue;
		static Persistent<Function> m_CallbackFunction;
		static Persistent<Function> m_SuspectsClearedCallback;
		static bool m_CallbackRegistered;
		static bool m_AllSuspectsClearedCallbackRegistered;
		static bool m_callbackRunning;
		static map<in_addr_t, Suspect*> m_suspects;

		static void InitNovaCallbackProcessing();
		static void CheckInitNova();

		static void NovaCallbackHandling(eio_req __attribute__((__unused__)) *req);
		static int AfterNovaCallbackHandling(eio_req __attribute__((__unused__)) *req);
		static void HandleNewSuspect(Suspect* suspect);
		static void HandleAllSuspectsCleared();
		static int HandleNewSuspectOnV8Thread(eio_req* req);
		static int HandleAllClearedOnV8Thread(eio_req* req);
		static void HandleCallbackError();

	public:

		static Persistent<FunctionTemplate> s_ct;

		static void Init(Handle<Object> target);
		static Handle<Value> CheckConnection(const Arguments __attribute__((__unused__)) & args);
		static Handle<Value> Shutdown(const Arguments __attribute__((__unused__)) & args);
		NovaNode();
		~NovaNode();
		
		static void SynchInternalList();
		static Handle<Value> New(const Arguments& args);
		static Handle<Value> getSuspectList(const Arguments& args);
		static Handle<Value> registerOnNewSuspect(const Arguments& args);
		static Handle<Value> registerOnAllSuspectsCleared(const Arguments& args);
		static void HandleOnNewSuspectWeakCollect(Persistent<Value> __attribute__((__unused__)) OnNewSuspectCallback, void __attribute__((__unused__)) * parameter);
};

