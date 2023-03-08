#include "node_nabto_device.h"

#include <nabto/nabto_device.h>

#include <iostream>


Napi::Object NodeNabtoDevice::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func =
      DefineClass(env,
                  "NabtoDevice",
                  {InstanceMethod("getVersion", &NodeNabtoDevice::GetVersion),
                //    InstanceMethod("createPrivateKey", &NodeNabtoDevice::CreatePrivateKey),
                //    InstanceMethod("setLogLevel", &NodeNabtoDevice::SetLogLevel),
                //    InstanceMethod("setLogCallback", &NodeNabtoDevice::SetLogCallback),
                   InstanceMethod("stop", &NodeNabtoDevice::Stop)
                   });

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NabtoDevice", func);
  return exports;
}

NodeNabtoDevice::NodeNabtoDevice(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NodeNabtoDevice>(info) {

  nabtoDevice_ = nabto_device_new();
}

NodeNabtoDevice::~NodeNabtoDevice()
{
    nabto_device_free(nabtoDevice_);
}

void NodeNabtoDevice::Finalize(Napi::Env env)
{
}

void NodeNabtoDevice::Stop(const Napi::CallbackInfo& info)
{
    // nabto_device_set_log_callback(nabtoDevice_, NULL, NULL);
    // if (logCallback_ != nullptr) {
    //     logCallback_.Release();
    // }
}

Napi::Value NodeNabtoDevice::GetVersion(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), nabto_device_version());
}
