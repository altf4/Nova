#ifndef NOVACONFIGBINDING_H
#define NOVACONFIGBINDING_H

#include <node.h>
#include "Config.h"

class NovaConfigBinding : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);

 private:
  NovaConfigBinding();
  ~NovaConfigBinding();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);

static v8::Handle<v8::Value> GetClassificationThreshold(const v8::Arguments& args);
static v8::Handle<v8::Value> GetClassificationTimeout(const v8::Arguments& args);
static v8::Handle<v8::Value> GetConfigFilePath(const v8::Arguments& args);
static v8::Handle<v8::Value> GetDataTTL(const v8::Arguments& args);
static v8::Handle<v8::Value> GetDoppelInterface(const v8::Arguments& args);
static v8::Handle<v8::Value> GetDoppelIp(const v8::Arguments& args);
static v8::Handle<v8::Value> GetEnabledFeatures(const v8::Arguments& args);
static v8::Handle<v8::Value> GetEnabledFeatureCount(const v8::Arguments& args);
static v8::Handle<v8::Value> GetEps(const v8::Arguments& args);
static v8::Handle<v8::Value> GetGotoLive(const v8::Arguments& args);
static v8::Handle<v8::Value> GetInterface(const v8::Arguments& args);
static v8::Handle<v8::Value> GetIsDmEnabled(const v8::Arguments& args);
static v8::Handle<v8::Value> GetIsTraining(const v8::Arguments& args);
static v8::Handle<v8::Value> GetK(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathCESaveFile(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathConfigHoneydUser(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathConfigHoneydHS(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathPcapFile(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathTrainingCapFolder(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathTrainingFile(const v8::Arguments& args);
static v8::Handle<v8::Value> GetReadPcap(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSaMaxAttempts(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSaPort(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSaSleepDuration(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSaveFreq(const v8::Arguments& args);
static v8::Handle<v8::Value> GetTcpCheckFreq(const v8::Arguments& args);
static v8::Handle<v8::Value> GetTcpTimout(const v8::Arguments& args);
static v8::Handle<v8::Value> GetThinningDistance(const v8::Arguments& args);
static v8::Handle<v8::Value> GetKey(const v8::Arguments& args);
static v8::Handle<v8::Value> GetNeighbors(const v8::Arguments& args);
static v8::Handle<v8::Value> GetGroup(const v8::Arguments& args);
static v8::Handle<v8::Value> GetLoggerPreferences(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSMTPAddr(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSMTPDomain(const v8::Arguments& args);
//static v8::Handle<v8::Value> GetSMTPEmailRecipients(const v8::Arguments& args);
//static v8::Handle<v8::Value> GetSMTPPort(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathBinaries(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathWriteFolder(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathReadFolder(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathIcon(const v8::Arguments& args);
static v8::Handle<v8::Value> GetPathHome(const v8::Arguments& args);
static v8::Handle<v8::Value> GetSqurtEnabledFeatures(const v8::Arguments& args);
static v8::Handle<v8::Value> GetHaystackStorage(const v8::Arguments& args);
static v8::Handle<v8::Value> GetUserPath(const v8::Arguments& args);



  double counter_;
  Nova::Config *m_conf;
};

#endif
