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
    arm(false);
  }
};


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
                      InstanceMethod("getConfiguration", &NodeNabtoDevice::GetConfiguration),
                      InstanceMethod("setBasestationAttach", &NodeNabtoDevice::SetBasestationAttach),
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

  if (!opts.Get("productId").IsString() ||
      !opts.Get("deviceId").IsString() ||
      !opts.Get("privateKey").IsString())
  {
    Napi::TypeError::New(info.Env(), "Invalid parameter, productId, deviceId, privateKey must be strings").ThrowAsJavaScriptException();
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

  if (opts.Has("serverUrl") && opts.Get("serverUrl").IsString())
  {
    ec = nabto_device_set_server_url(nabtoDevice_, opts.Get("serverUrl").ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set server URL with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("serverPort") && opts.Get("serverPort").IsNumber())
  {
    ec = nabto_device_set_server_port(nabtoDevice_, opts.Get("serverUrl").ToNumber().Uint32Value());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set server port with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("rootCerts") && opts.Get("rootCerts").IsString())
  {
    ec = nabto_device_set_root_certs(nabtoDevice_, opts.Get("rootCerts").ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set root certs with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("appName") && opts.Get("appName").IsString())
  {
    ec = nabto_device_set_app_name(nabtoDevice_, opts.Get("appName").ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set App name with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("appVersion") && opts.Get("appVersion").IsString())
  {
    ec = nabto_device_set_app_version(nabtoDevice_, opts.Get("appVersion").ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set App version with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("localPort") && opts.Get("localPort").IsNumber())
  {
    ec = nabto_device_set_local_port(nabtoDevice_, opts.Get("localPort").ToNumber().Uint32Value());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set local port with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("p2pPort") && opts.Get("p2pPort").IsNumber())
  {
    ec = nabto_device_set_p2p_port(nabtoDevice_, opts.Get("p2pPort").ToNumber().Uint32Value());
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set p2p port with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }

  if (opts.Has("enableMdns") && opts.Get("enableMdns").IsBoolean() && opts.Get("enableMdns").ToBoolean().Value())
  {
    ec = nabto_device_enable_mdns(nabtoDevice_);
    if (ec != NABTO_DEVICE_EC_OK)
    {
      std::string msg = "Failed to set p2p port with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
      return;
    }
  }
}

Napi::Value NodeNabtoDevice::GetConfiguration(const Napi::CallbackInfo& info)
{
  Napi::Object conf = Napi::Object::New(info.Env());
  const char* prod = nabto_device_get_product_id(nabtoDevice_);
  conf.Set("productId", prod);
  const char* dev = nabto_device_get_device_id(nabtoDevice_);
  conf.Set("deviceId", dev);
  const char* appName = nabto_device_get_app_name(nabtoDevice_);
  conf.Set("appName", appName);
  const char* appVer = nabto_device_get_app_version(nabtoDevice_);
  conf.Set("appVersion", appVer);

  uint16_t port = 0;
  NabtoDeviceError ec = nabto_device_get_local_port(nabtoDevice_, &port);
  conf.Set("localPort", port);
  port = 0;
  ec = nabto_device_get_p2p_port(nabtoDevice_, &port);
  conf.Set("p2pPort", port);

  char* fp;
  ec = nabto_device_get_device_fingerprint(nabtoDevice_, &fp);
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return conf;
  }
  conf.Set("deviceFingerprint", fp);
  nabto_device_string_free(fp);

  return conf;
}


Napi::Value NodeNabtoDevice::CreatePrivateKey(const Napi::CallbackInfo& info) {
  char* key;

  NabtoDeviceError ec = nabto_device_create_private_key(nabtoDevice_, &key);
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return Napi::Value();
  }
  Napi::Value retVal = Napi::String::New(info.Env(), key);
  nabto_device_string_free(key);
  return retVal;
}

void NodeNabtoDevice::SetBasestationAttach(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  Napi::Value result;
  if (length <= 0 || !info[0].IsBoolean() ) {
    Napi::TypeError::New(info.Env(), "Boolean expected").ThrowAsJavaScriptException();
    return;
  }

  NabtoDeviceError ec = nabto_device_set_basestation_attach(nabtoDevice_, info[0].ToBoolean().Value());
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return;
  }
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

    auto l = new LogMessage(log->message, nabto_device_log_severity_as_string(log->severity));
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


