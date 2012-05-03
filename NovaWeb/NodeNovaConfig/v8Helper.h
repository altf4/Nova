#ifndef __V8_HELPER
#define __V8_HELPER

#include "v8.h"
#include "v8-convert.hpp"
#include <node.h>

#include <string>
#include <iostream>
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
template <typename NATIVE_RETURN,  NATIVE_RETURN (*F)()> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments __attribute((__unused__)) & args)
{
    using namespace v8;
	HandleScope scope;
    
    Handle<v8::Value> result = cvv8::CastToJS(F());
	return scope.Close(result);
}   

// Invocation of standalone (non-member) functions,
// one-argument version version.
// See note above regarding partial specialization, if it is ever required.
template <typename NATIVE_RETURN, typename V8_P1, typename NATIVE_P1, NATIVE_RETURN (*F)(NATIVE_P1)> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>( args[0] );

    Handle<v8::Value> result = cvv8::CastToJS(F(p1));
    return scope.Close(result);
}







// Invocation of member methods,
// zero-argument version.
//
// Funny template approach is used to enable emulated partial specialization.
template <typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
struct InvokeMethod_impl
{
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
    T* nativeHandler = Unwrap<T>(args.This());

    Handle<v8::Value> result = cvv8::CastToJS(  (  (nativeHandler)->*(F)  )()  );
    return scope.Close(result);
} 
};

// Invocation of member methods,
// one-argument version.
template <typename NATIVE_RETURN, typename T, typename V8_P1, typename NATIVE_P1, NATIVE_RETURN (T::*F)(NATIVE_P1)> 
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

    Handle<v8::Value> result = cvv8::CastToJS(  (  (nativeHandler)->*(F)  )(p1)  );
    return scope.Close(result);
} 
};

// Invocation of member methods,
// zero argument version.
// Specialization for in_addr return type.
template<typename T, in_addr (T::*F)(void)>
struct InvokeMethod_impl<in_addr, T, F>
{
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
    T* nativeHandler = Unwrap<T>(args.This());

    in_addr addr = (nativeHandler->*(F))();
    char* addrAsString = inet_ntoa(addr);
    Local<String> result = Local<String>::New( String::New( addrAsString ));
    return scope.Close(result);
} 
};

// Invocation of member methods,
// zero argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    return InvokeMethod_impl<NATIVE_RETURN, T, F >::InvokeMethod(args);
}

// Invocation of member methods,
// one argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, typename V8_P1, typename NATIVE_P1, NATIVE_RETURN (T::*F)(NATIVE_P1)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
    return InvokeMethod_impl_1<NATIVE_RETURN, T, V8_P1, NATIVE_P1, F >::InvokeMethod(args);
}











// Invocation of member methods,
// zero-argument version.
//
// Funny template approach is used to enable emulated partial specialization.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, NATIVE_RETURN (CHILD_TYPE::*F)(void)> 
struct InvokeWrappedMethod_impl
{
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
	
	T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());
	CHILD_TYPE *handler = nativeHandler->GetChild();

    Handle<v8::Value> result = cvv8::CastToJS(  (  (handler)->*(F)  )()  );
    return scope.Close(result);
} 
};


// Invocation of member methods,
// one-argument version.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1)> 
struct InvokeWrappedMethod_impl_1
{
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
    using namespace v8;
    HandleScope scope;
	T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }
    
	NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>(args[0]);
	
	
	CHILD_TYPE *handler = nativeHandler->GetChild();

    Handle<v8::Value> result = cvv8::CastToJS(  (  (handler)->*(F)  )(p1)  );
    return scope.Close(result);
} 
};

// Invocation of member methods,
// zero argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, NATIVE_RETURN (CHILD_TYPE::*F)(void)> 
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
    return InvokeWrappedMethod_impl<NATIVE_RETURN, T, CHILD_TYPE, F >::InvokeWrappedMethod(args);
}

// Invocation of member methods,
// one argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1)> 
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
    return InvokeWrappedMethod_impl_1<NATIVE_RETURN, T, CHILD_TYPE, NATIVE_P1, F >::InvokeWrappedMethod(args);
}



#endif // __V8_HELPER

