#ifndef HONEYDCONFIGBINDING_H
#define HONEYDCONFIGBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "HoneydConfiguration/HoneydConfiguration.h"


class HoneydConfigBinding : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);
	Nova::HoneydConfiguration* GetChild();

private:
	HoneydConfigBinding();
	~HoneydConfigBinding();
	Nova::HoneydConfiguration *m_conf;


	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	static v8::Handle<v8::Value> AddNewNodes(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddNewNode(const v8::Arguments& args);

	static v8::Handle<v8::Value> GetNode(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetProfile(const v8::Arguments& args);

	static v8::Handle<v8::Value> GetPorts(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddPort(const v8::Arguments& args);

	static v8::Handle<v8::Value> SaveAll(const v8::Arguments& args);
	static v8::Handle<v8::Value> DeleteProfile(const v8::Arguments& args);
};

#endif
