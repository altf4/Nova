#ifndef __V8_HELPER
#define __V8_HELPER

#include "v8.h"
#include "v8-convert.hpp"

#include <string>
#include <arpa/inet.h>

template <typename T> T* Unwrap(v8::Handle<v8::Object> obj, int fieldIndex=0)
{
    using namespace v8;
    Handle<External> objectHandle = Handle<External>::Cast(obj->GetInternalField(fieldIndex));
    void* ptr = objectHandle->Value();
    return static_cast<T*>(ptr);
}

// For invocation of standalone (non-member) functions,
// zero-argument version.
// NOTE: if partial specialization will be required for non-member functions, duplicate the
// "funny template approach" below that is set up for emulation of partial specialization
// of member functions.  Everything would be the same except without the 
// "typename T" and the "T::"
template <typename V8_RETURN, typename NATIVE_RETURN,  NATIVE_RETURN (*F)()> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments __attribute((__unused__)) & args)
{
    using namespace v8;
	HandleScope scope;
    
	Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( F() ));
	return scope.Close(result);
}   

// Invocation of standalone (non-member) functions,
// one-argument version version.
// See note above regarding partial specialization, if it is ever required.
template <typename V8_RETURN, typename NATIVE_RETURN, typename V8_P1, typename NATIVE_P1, NATIVE_RETURN (*F)(NATIVE_P1)> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>( args[0] );

    Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( F(p1) ));
    return scope.Close(result);
}

// Invocation of member methods,
// zero-argument version.
//
// Funny template approach is used to enable emulated partial specialization.
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

// Invocation of member methods,
// one-argument version.
template <typename V8_RETURN, typename NATIVE_RETURN, typename T, typename V8_P1, typename NATIVE_P1, NATIVE_RETURN (T::*F)(NATIVE_P1)> 
struct InvokeMethod_impl_1
{
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
    T* nativeHandler = Unwrap<T>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>(args[0]);

    Local<V8_RETURN> result = Local<V8_RETURN>::New( V8_RETURN::New( ((nativeHandler)->*(F))(p1) ));
    return scope.Close(result);
} 
};


// Invocation of member methods, 
// zero-argument version.
// Specialization for std::string return type.
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

// Invocation of member methods,
// zero argument version.
// Specialization for in_addr return type.
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

// Invocation of member methods,
// zero argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename V8_RETURN, typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    return InvokeMethod_impl<V8_RETURN, NATIVE_RETURN, T, F >::InvokeMethod(args);
}

// Invocation of member methods,
// one argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename V8_RETURN, typename NATIVE_RETURN, typename T, typename V8_P1, typename NATIVE_P1, NATIVE_RETURN (T::*F)(NATIVE_P1)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    return InvokeMethod_impl_1<V8_RETURN, NATIVE_RETURN, T, V8_P1, NATIVE_P1, F >::InvokeMethod(args);
}


#endif // __V8_HELPER
