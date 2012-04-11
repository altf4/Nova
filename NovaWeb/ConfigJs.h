#ifndef __CONFIGJS_H
#define __CONFIGJS_H

#include <v8.h>
#include "v8Helper.h"
#include "Config.h"

class ConfigJs
{

public:
    static v8::Handle<v8::Object> WrapConfig(Nova::Config* config);


/* The following is how you'd do it if you wanted to return index-based
        if( args.Length() < 1 )
        {
            return ThrowException(Exception::TypeError(String::New("Must provide index index")));
        }
        else
        {
            // just return the one
            int fIndex = cvv8::CastFromJS<int>(args[0]);
            double value = config->GetFeatureSet().m_features[fIndex];
            return scope.Close(Number::New(value));
        }
*/

private:
    static v8::Persistent<v8::FunctionTemplate> m_ConfigTemplate;

};


#endif // __CONFIGJS_H
