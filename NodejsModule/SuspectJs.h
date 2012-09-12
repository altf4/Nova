#ifndef __SUSPECTJS_H
#define __SUSPECTJS_H

#include <v8.h>
#include "v8Helper.h"
#include "Suspect.h"
#include "FeatureSet.h"
#include <v8-convert.hpp>

class SuspectJs
{

public:
    static v8::Handle<v8::Object> WrapSuspect(Nova::Suspect* suspect);


    // special handler for JS GetFeatures()
    static v8::Handle<v8::Value> GetFeatures(const v8::Arguments& args)
    {
        using namespace v8;
        using namespace Nova;
        HandleScope scope;
        Local<Object> self = args.Holder();
        Local<External> suspectPtr = Local<External>::Cast(self->GetInternalField(0));
        Suspect* suspect = reinterpret_cast<Suspect*>(suspectPtr->Value());
        if( suspect == 0 )
        {
            return ThrowException(Exception::TypeError(String::New("Internal error unwrapping Suspect")));
        }

        FeatureSet featureSet = suspect->GetFeatureSet();
		// Wrap the features in a vector
		vector<double> features(featureSet.m_features, featureSet.m_features + sizeof(featureSet.m_features) / sizeof(double));
        
		return scope.Close(cvv8::CastToJS(features));
    }

private:
    static v8::Persistent<v8::FunctionTemplate> m_SuspectTemplate;

};


#endif // __SUSPECTJS_H
