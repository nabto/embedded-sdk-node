#include "node_nabto_device.h"
#include "future.h"

#include <nabto/nabto_device.h>

#include <iostream>

class StartFutureContext : public FutureContext
{
public:
  StartFutureContext(NabtoDevice *device, Napi::Env env) : FutureContext(device, env)
  {
    nabto_device_start(device_, future_);
    arm();
  }
};

static std::string
logSeverityToString(NabtoDeviceLogLevel s)
{
  switch(s) {
    case NABTO_DEVICE_LOG_FATAL: return "fatal";
    case NABTO_DEVICE_LOG_ERROR: return "error";
    case NABTO_DEVICE_LOG_WARN: return "warn";
    case NABTO_DEVICE_LOG_INFO: return "info";
    case NABTO_DEVICE_LOG_TRACE: return "trace";
    default: return "trace";
  }
}

Napi::Object NodeNabtoDevice::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func =
      DefineClass(env,
                  "NabtoDevice",
                  {
                      InstanceMethod("getVersion", &NodeNabtoDevice::GetVersion),
                      InstanceMethod("setOptions", &NodeNabtoDevice::SetOptions),
                      InstanceMethod("createPrivateKey", &NodeNabtoDevice::CreatePrivateKey),
                      InstanceMethod("setLogLevel", &NodeNabtoDevice::SetLogLevel),
                      InstanceMethod("setLogCallback", &NodeNabtoDevice::SetLogCallback),
                      InstanceMethod("stop", &NodeNabtoDevice::Stop),
                      InstanceMethod("start", &NodeNabtoDevice::Start),
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
    nabto_device_set_log_callback(nabtoDevice_, NULL, NULL);
    if (logCallback_ != nullptr) {
        logCallback_.Release();
    }
}

Napi::Value NodeNabtoDevice::Start(const Napi::CallbackInfo& info)
{
  StartFutureContext* sfc = new StartFutureContext(nabtoDevice_, info.Env());
  return sfc->Promise();
}

Napi::Value NodeNabtoDevice::GetVersion(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), nabto_device_version());
}

void NodeNabtoDevice::SetOptions(const Napi::CallbackInfo& info) {
  int length = info.Length();
  Napi::Value result;
  if (length <= 0 || !info[0].IsObject() ) {
    Napi::TypeError::New(info.Env(), "Object expected").ThrowAsJavaScriptException();
    return;
  }
  Napi::Object opts = info[0].ToObject();

  if (!opts.Has("productId") ||
      !opts.Has("deviceId") ||
      !opts.Has("privateKey")) {
    Napi::TypeError::New(info.Env(), "Missing parameter, productId, deviceId, privateKey is required").ThrowAsJavaScriptException();
    return;
  }

  NabtoDeviceError ec = nabto_device_set_product_id(nabtoDevice_, opts.Get("productId").ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    std::string msg = "Failed to set Product ID with error: ";
    msg += nabto_device_error_get_message(ec);
    Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
    return;
  }

  ec = nabto_device_set_device_id(nabtoDevice_, opts.Get("deviceId").ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    std::string msg = "Failed to set Device ID with error: ";
    msg += nabto_device_error_get_message(ec);
    Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
    return;
  }

  ec = nabto_device_set_private_key(nabtoDevice_, opts.Get("privateKey").ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    std::string msg = "Failed to set private key with error: ";
    msg += nabto_device_error_get_message(ec);
    Napi::Error::New(info.Env(),  msg).ThrowAsJavaScriptException();
    return;
  }

  // TODO: serverUrl, serverPort, rootCert, appName, appVersion, localPort, p2pPort
}

Napi::Value NodeNabtoDevice::CreatePrivateKey(const Napi::CallbackInfo& info) {
  char* key;

  NabtoDeviceError ec = nabto_device_create_private_key(nabtoDevice_, &key);
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();

  }
  Napi::Value retVal = Napi::String::New(info.Env(), key);
  nabto_device_string_free(key);
  return retVal;
}


void NodeNabtoDevice::SetLogLevel(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  Napi::Value result;
  if (length <= 0 || !info[0].IsString() ) {
    Napi::TypeError::New(info.Env(), "String expected").ThrowAsJavaScriptException();
    return;
  }

  NabtoDeviceError ec = nabto_device_set_log_level(nabtoDevice_, info[0].ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::Error::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
  }
}


void CallJs(Napi::Env env, Napi::Function callback, Context *context, LogMessage *data)
{

  // Is the JavaScript environment still available to call into, eg. the TSFN is
  // not aborted
  if (env != nullptr) {
    // On Node-API 5+, the `callback` parameter is optional; however, this example
    // does ensure a callback is provided.
    if (callback != nullptr) {
        Napi::Object o = Napi::Object::New(callback.Env());
        o.Set("message", data->message_);
        o.Set("severity", data->severity_);
      callback.Call({o});
    }
  }
  if (data != nullptr) {
    // We're finished with the data.
    delete data;
  }
}

void NodeNabtoDevice::LogCallback(NabtoDeviceLogMessage* log, void* userData)
{
    NodeNabtoDevice* device = static_cast<NodeNabtoDevice*>(userData);

    auto l = new LogMessage(log->message, logSeverityToString(log->severity));
    device->logCallback_.NonBlockingCall(l);
}

void NodeNabtoDevice::SetLogCallback(const Napi::CallbackInfo& info)
{
  //Napi::Env env = info.Env();
  Napi::Function cb = info[0].As<Napi::Function>();
  //Context *context = new Napi::Reference<Napi::Value>(Persistent(info.This()));
  logCallback_ = LogCallbackFunction::New(cb.Env(), cb, "TSFN", 0, 1, NULL, [](Napi::Env, void*,
        Context *ctx) { // Finalizer used to clean threads up
        //delete ctx;
      });
    nabto_device_set_log_callback(nabtoDevice_, NodeNabtoDevice::LogCallback, this);
}


