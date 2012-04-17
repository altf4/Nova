#ifndef __SUSPECTJS_H
#define __SUSPECTJS_H

#include <v8.h>
#include "v8Helper.h"
#include "Suspect.h"
#include "FeatureSet.h"

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

        Local<Object> ret = Object::New();
        FeatureSet featureSet = suspect->GetFeatureSet();
        // Someone really needs to fix the design of class FeatureSet
        ret->Set(String::New("ip_traffic_distribution"), Number::New(featureSet.m_features[0]));
        ret->Set(String::New("port_traffic_distribution"), Number::New(featureSet.m_features[1]));
        ret->Set(String::New("haystack_event_frequency"), Number::New(featureSet.m_features[2]));
        ret->Set(String::New("packet_size_deviation"), Number::New(featureSet.m_features[3]));
        ret->Set(String::New("packet_size_mean"), Number::New(featureSet.m_features[4]));
        ret->Set(String::New("distinct_ips"), Number::New(featureSet.m_features[5]));
        ret->Set(String::New("distinct_ports"), Number::New(featureSet.m_features[6]));
        ret->Set(String::New("packet_interval_mean"), Number::New(featureSet.m_features[7]));
        ret->Set(String::New("packet_interval_deviation"), Number::New(featureSet.m_features[8]));

        return scope.Close(ret);
        
/* The following is how you'd do it if you wanted to return index-based
        if( args.Length() < 1 )
        {
            return ThrowException(Exception::TypeError(String::New("Must provide index index")));
        }
        else
        {
            // just return the one
            int fIndex = cvv8::CastFromJS<int>(args[0]);
            double value = suspect->GetFeatureSet().m_features[fIndex];
            return scope.Close(Number::New(value));
        }
*/
    }

private:
    static v8::Persistent<v8::FunctionTemplate> m_SuspectTemplate;

};


#endif // __SUSPECTJS_H
