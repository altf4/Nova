#ifndef __SUSPECTJS_H
#define __SUSPECTJS_H

#include <v8.h>
#include "v8Helper.h"
#include "Suspect.h"

class SuspectJs
{

public:
    static v8::Handle<v8::Object> WrapSuspect(Nova::Suspect* suspect);



private:
    static v8::Persistent<v8::FunctionTemplate> m_SuspectTemplate;

};


#endif // __SUSPECTJS_H
