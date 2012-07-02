#ifndef TRAININGDUMPBINDING_H
#define TRAININGDUMPBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "TrainingDump.h"

class TrainingDumpBinding : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);
	TrainingDump * GetChild();

private:
	TrainingDumpBinding();
	~TrainingDumpBinding();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	TrainingDump *m_db;
};

#endif
