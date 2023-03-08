#pragma once

#include <napi.h>
#include <nabto/nabto_device.h>



using Context = void;
//void CallJs(Napi::Env env, Napi::Function callback, Context *context, LogMessage *data);
//typedef Napi::TypedThreadSafeFunction<Context, LogMessage, CallJs> LogCallbackFunction;

class NodeNabtoDevice : public Napi::ObjectWrap<NodeNabtoDevice> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  virtual void Finalize(Napi::Env env);
  NodeNabtoDevice(const Napi::CallbackInfo& info);
  ~NodeNabtoDevice();
  void Stop(const Napi::CallbackInfo& info);

  NabtoDevice* getDevice() { return nabtoDevice_; }

 private:
//  static void LogCallback(const NabtoClientLogMessage* log, void* userData);
  Napi::Value GetVersion(const Napi::CallbackInfo& info);
//  Napi::Value CreatePrivateKey(const Napi::CallbackInfo& info);
//  void SetLogLevel(const Napi::CallbackInfo& info);
//  void SetLogCallback(const Napi::CallbackInfo& info);

  NabtoDevice* nabtoDevice_;
//  LogCallbackFunction logCallback_;
};
