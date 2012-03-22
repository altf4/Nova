#ifndef __V8_HELPER
#define __V8_HELPER

#include "v8.h"

#include <string>
#include <arpa/inet.h>

template <typename T> T* Unwrap(v8::Handle<v8::Object> obj, int fieldIndex=0)
{
    using namespace v8;
    Handle<External> objectHandle = Handle<External>::Cast(obj->GetInternalField(fieldIndex));
    void* ptr = objectHandle->Value();
    return static_cast<T*>(ptr);
}


// Funny template approach to enable emulated partial specialization
template <typename V8_RETURN, typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
struct InvokeMethod_impl
{
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
    T* nativeHandler = Unwrap<T>(args.This());

    Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( ((nativeHandler)->*(F))() ));
    return scope.Close(result);
} 
};

// Specialization for std::string
template<typename V8_RETURN, typename T, std::string (T::*F)(void)>
struct InvokeMethod_impl<V8_RETURN, std::string, T, F>
{
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
    T* nativeHandler = Unwrap<T>(args.This());

    Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( ((nativeHandler)->*(F))().c_str() ));
    return scope.Close(result);
} 
};

// Specialization for in_addr
template<typename V8_RETURN, typename T, in_addr (T::*F)(void)>
struct InvokeMethod_impl<V8_RETURN, in_addr, T, F>
{
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
    T* nativeHandler = Unwrap<T>(args.This());

    in_addr addr = (nativeHandler->*(F))();
    char* addrAsString = inet_ntoa(addr);
    Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( addrAsString ));
    return scope.Close(result);
} 
};

// The wrapper template function to invoke the funny static class members
// above who provide partial specialization where required.
template <typename V8_RETURN, typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    return InvokeMethod_impl<V8_RETURN, NATIVE_RETURN, T, F >::InvokeMethod(args);
}


#endif // __V8_HELPER
