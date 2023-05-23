#include "node_nabto_device.h"
#include "future.h"

#include <nabto/nabto_device.h>
#include <nabto/nabto_device_experimental.h>

#include <iostream>

std::vector<uint8_t> bytesFromHex(std::string hex);

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
    DefineClass(
      env,
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
        InstanceMethod("notifyDeviceEvent", &NodeNabtoDevice::NotifyDeviceEvent),
        InstanceMethod("getCurrentDeviceEvent", & NodeNabtoDevice::GetCurrentDeviceEvent),
        InstanceMethod("notifyConnectionEvent", &NodeNabtoDevice::NotifyConnectionEvent),
        InstanceMethod("getCurrentConnectionEvent", &NodeNabtoDevice::GetCurrentConnectionEvent),
        InstanceMethod("getCurrentConnectionRef", &NodeNabtoDevice::GetCurrentConnectionRef),
        InstanceMethod("connectionGetClientFingerprint", &NodeNabtoDevice::ConnectionGetClientFingerprint),
        InstanceMethod("connectionIsLocal", &NodeNabtoDevice::ConnectionIsLocal),
        InstanceMethod("connectionIsPasswordAuthenticated", &NodeNabtoDevice::ConnectionIsPasswordAuthenticated),
        InstanceMethod("connectionGetPasswordAuthUsername", &NodeNabtoDevice::ConnectionGetPasswordAuthUsername),
        InstanceMethod("mdnsAddSubtype", &NodeNabtoDevice::MdnsAddSubtype),
        InstanceMethod("mdnsAddTxtItem", &NodeNabtoDevice::MdnsAddTxtItem),
        InstanceMethod("createServerConnectToken", &NodeNabtoDevice::CreateServerConnectToken),
        InstanceMethod("addServerConnectToken", &NodeNabtoDevice::AddServerConnectToken),
        InstanceMethod("areServerConnectTokensSync", &NodeNabtoDevice::AreServerConnectTokensSync),
        InstanceMethod("addTcpTunnelService", &NodeNabtoDevice::AddTcpTunnelService),
        InstanceMethod("removeTcpTunnelService", &NodeNabtoDevice::RemoveTcpTunnelService),
        InstanceMethod("setRawPrivateKey", &NodeNabtoDevice::SetRawPrivateKey),
      });

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NabtoDevice", func);
  return exports;
}

NodeNabtoDevice::NodeNabtoDevice(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NodeNabtoDevice>(info) {
  devEvents_ = NULL;
  connEvents_ = NULL;
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
  if (devEvents_ != NULL) {
    devEvents_->stop();
  }
  if (connEvents_ != NULL) {
    connEvents_->stop();
  }
  nabto_device_stop(nabtoDevice_);
  nabto_device_set_log_callback(nabtoDevice_, NULL, NULL);
  if (logCallback_ != nullptr)
  {
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
  if (length <= 0 || !info[0].IsObject() ) {
    Napi::TypeError::New(info.Env(), "Object expected").ThrowAsJavaScriptException();
    return;
  }
  Napi::Object opts = info[0].ToObject();

  if (!opts.Has("productId") ||
      !opts.Has("deviceId")) {
    Napi::TypeError::New(info.Env(), "Missing parameter, productId, deviceId is required").ThrowAsJavaScriptException();
    return;
  }

  if (!opts.Get("productId").IsString() ||
      !opts.Get("deviceId").IsString())
  {
    Napi::TypeError::New(info.Env(), "Invalid parameter, productId, deviceId must be strings").ThrowAsJavaScriptException();
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

  if (opts.Has("privateKey") && opts.Get("privateKey").IsString())
  {
    ec = nabto_device_set_private_key(nabtoDevice_, opts.Get("privateKey").ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK) {
      std::string msg = "Failed to set private key with error: ";
      msg += nabto_device_error_get_message(ec);
      Napi::Error::New(info.Env(),  msg).ThrowAsJavaScriptException();
      return;
    }
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
    ec = nabto_device_set_server_port(nabtoDevice_, opts.Get("serverPort").ToNumber().Uint32Value());
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
  if (appName != NULL) {
    conf.Set("appName", appName);
  }
  const char* appVer = nabto_device_get_app_version(nabtoDevice_);
  if (appVer != NULL) {
    conf.Set("appVersion", appVer);
  }

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

/************ MDNS *********/
void NodeNabtoDevice::MdnsAddSubtype(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 0 || !info[0].IsString() ) {
    Napi::TypeError::New(info.Env(), "String expected").ThrowAsJavaScriptException();
    return;
  }

  NabtoDeviceError ec = nabto_device_mdns_add_subtype(nabtoDevice_, info[0].ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return;
  }
}

void NodeNabtoDevice::MdnsAddTxtItem(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 1 || !info[0].IsString() || !info[1].IsString() ) {
    Napi::TypeError::New(info.Env(), "2 Strings expected").ThrowAsJavaScriptException();
    return;
  }

  NabtoDeviceError ec = nabto_device_mdns_add_txt_item(nabtoDevice_, info[0].ToString().Utf8Value().c_str(), info[1].ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return;
  }
}

/************ SCT *********/

Napi::Value NodeNabtoDevice::CreateServerConnectToken(const Napi::CallbackInfo& info) {
  char* sct;

  NabtoDeviceError ec = nabto_device_create_server_connect_token(nabtoDevice_, &sct);
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return Napi::Value();
  }
  Napi::Value retVal = Napi::String::New(info.Env(), sct);
  nabto_device_string_free(sct);
  return retVal;
}

void NodeNabtoDevice::AddServerConnectToken(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 0 || !info[0].IsString() ) {
    Napi::TypeError::New(info.Env(), "String expected").ThrowAsJavaScriptException();
    return;
  }

  NabtoDeviceError ec = nabto_device_add_server_connect_token(nabtoDevice_, info[0].ToString().Utf8Value().c_str());
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return;
  }
}

Napi::Value NodeNabtoDevice::AreServerConnectTokensSync(const Napi::CallbackInfo& info)
{
  NabtoDeviceError ec = nabto_device_are_server_connect_tokens_synchronized(nabtoDevice_);

  if (ec != NABTO_DEVICE_EC_OK) {
    return Napi::Boolean::New(info.Env(), false);
  }
  return Napi::Boolean::New(info.Env(), true);
}


/************ DEVICE EVENTS *********/
Napi::Value NodeNabtoDevice::NotifyDeviceEvent(const Napi::CallbackInfo& info)
{
  if (devEvents_ == NULL) {
    devEvents_ = new DeviceEventFutureContext(nabtoDevice_, info.Env());
  } else {
    devEvents_->rearm();
  }
  return devEvents_->Promise();
}

Napi::Value NodeNabtoDevice::GetCurrentDeviceEvent(const Napi::CallbackInfo& info)
{
  return Napi::String::New(info.Env(), devEvents_->getEventString());
}


/************ CONNECTION EVENTS *********/
Napi::Value NodeNabtoDevice::NotifyConnectionEvent(const Napi::CallbackInfo& info)
{
  if (connEvents_ == NULL) {
    connEvents_ = new ConnectionEventFutureContext(nabtoDevice_, info.Env());
  } else {
    connEvents_->rearm();
  }
  return connEvents_->Promise();
}

Napi::Value NodeNabtoDevice::GetCurrentConnectionEvent(const Napi::CallbackInfo& info)
{
  return Napi::String::New(info.Env(), connEvents_->getEventString());
}

Napi::Value NodeNabtoDevice::GetCurrentConnectionRef(const Napi::CallbackInfo& info)
{
  return Napi::Number::New(info.Env(), connEvents_->getConnectionRef());
}

/*************** CONNECTIONS ************/
Napi::Value NodeNabtoDevice::ConnectionGetClientFingerprint(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 0 || !info[0].IsNumber() ) {
    Napi::TypeError::New(info.Env(), "Invalid ConnectionRef").ThrowAsJavaScriptException();
    return Napi::Value();
  }

  char* fp;
  NabtoDeviceError ec = nabto_device_connection_get_client_fingerprint(nabtoDevice_, info[0].ToNumber().Uint32Value(), &fp);
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return Napi::Value();
  }
  Napi::Value retVal = Napi::String::New(info.Env(), fp);
  nabto_device_string_free(fp);
  return retVal;
}

Napi::Value NodeNabtoDevice::ConnectionIsLocal(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 0 || !info[0].IsNumber() ) {
    Napi::TypeError::New(info.Env(), "Invalid ConnectionRef").ThrowAsJavaScriptException();
    return Napi::Value();
  }

  bool local = nabto_device_connection_is_local(nabtoDevice_, info[0].ToNumber().Uint32Value());

  return Napi::Boolean::New(info.Env(), local);
}

Napi::Value NodeNabtoDevice::ConnectionIsPasswordAuthenticated(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 0 || !info[0].IsNumber() ) {
    Napi::TypeError::New(info.Env(), "Invalid ConnectionRef").ThrowAsJavaScriptException();
    return Napi::Value();
  }

  bool result = nabto_device_connection_is_password_authenticated(nabtoDevice_, info[0].ToNumber().Uint32Value());

  return Napi::Boolean::New(info.Env(), result);
}

Napi::Value NodeNabtoDevice::ConnectionGetPasswordAuthUsername(const Napi::CallbackInfo& info)
{
  int length = info.Length();
  if (length <= 0 || !info[0].IsNumber() ) {
    Napi::TypeError::New(info.Env(), "Invalid ConnectionRef").ThrowAsJavaScriptException();
    return Napi::Value();
  }

  char* name;
  NabtoDeviceError ec = nabto_device_connection_get_password_authentication_username(nabtoDevice_, info[0].ToNumber().Uint32Value(), &name);
  if (ec != NABTO_DEVICE_EC_OK) {
    Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
    return Napi::Value();
  }
  Napi::Value retVal = Napi::String::New(info.Env(), name);
  nabto_device_string_free(name);
  return retVal;
}

/********** TUNNELS ********/
void NodeNabtoDevice::AddTcpTunnelService(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 4)
    {
        Napi::TypeError::New(env, "Expected 4 arguments: serviceId, serviceType, host, port").ThrowAsJavaScriptException();
        return;
    }
    Napi::Value id = info[0];
    Napi::Value type = info[1];
    Napi::Value host = info[2];
    Napi::Value port = info[3];

    if(!id.IsString()) {
         Napi::TypeError::New(env, "First arg expected serviceId string").ThrowAsJavaScriptException();
        return;
    }
    if(!type.IsString()) {
         Napi::TypeError::New(env, "Second arg expected serviceType string").ThrowAsJavaScriptException();
        return;
    }
    if(!host.IsString()) {
         Napi::TypeError::New(env, "Third arg expected host string").ThrowAsJavaScriptException();
        return;
    }
    if(!port.IsNumber()) {
         Napi::TypeError::New(env, "Fourth arg expected port number").ThrowAsJavaScriptException();
        return;
    }
    NabtoDeviceError ec = nabto_device_add_tcp_tunnel_service(
      nabtoDevice_,
      id.ToString().Utf8Value().c_str(),
      type.ToString().Utf8Value().c_str(),
      host.ToString().Utf8Value().c_str(),
      port.ToNumber().Uint32Value());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
}

void NodeNabtoDevice::RemoveTcpTunnelService(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1)
    {
        Napi::TypeError::New(env, "Expected 1 arguments: serviceId").ThrowAsJavaScriptException();
        return;
    }
    Napi::Value id = info[0];
    if(!id.IsString()) {
         Napi::TypeError::New(env, "First arg expected serviceId string").ThrowAsJavaScriptException();
        return;
    }
    NabtoDeviceError ec = nabto_device_remove_tcp_tunnel_service(
      nabtoDevice_,
      id.ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
}

/********** EXPERIMENTAL ******/
void NodeNabtoDevice::SetRawPrivateKey(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1)
    {
        Napi::TypeError::New(env, "Expected 1 arguments: key").ThrowAsJavaScriptException();
        return;
    }
    Napi::Value key = info[0];
    if(!key.IsString()) {
         Napi::TypeError::New(env, "First arg expected key string").ThrowAsJavaScriptException();
        return;
    }
    std::vector<uint8_t> rawKey = bytesFromHex(key.ToString().Utf8Value());

    NabtoDeviceError ec = nabto_device_set_private_key_secp256r1(
      nabtoDevice_, rawKey.data(), rawKey.size());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }

}

/********* LOGGING ****/
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


uint8_t byteFromChar(char c)
{
  if (c >= '0' && c <= '9')
    return (c - '0');
  else if (c >= 'A' && c <= 'F')
    return (10 + (c - 'A'));
  else if (c >= 'a' && c <= 'f')
    return (10 + (c - 'a'));
  else
    return 0;
}

std::vector<uint8_t> bytesFromHex(std::string hex)
{
  std::vector<uint8_t> res;

  for (size_t i = 0; i < (hex.size()-1); i=i+2) {
    uint8_t v1 = byteFromChar(hex[i]);
    uint8_t v2 = byteFromChar(hex[i+1]);
    res.push_back((v2 + (v1 << 4)));
  }
  return res;
}


/********* Ice Servers Request ****/



class IceFutureContext : public FutureContext
{
public:
  IceFutureContext(NabtoDevice* device, std::string identifier, NabtoDeviceIceServersRequest* req, Napi::Env env) : FutureContext(device, env)
  {
    nabto_device_ice_servers_request_send(identifier.c_str(), req, future_);
    arm(false);
  }
};


Napi::Object IceServersRequest::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func =
    DefineClass(
      env,
      "IceServersRequest",
        {
          InstanceMethod("send", &IceServersRequest::Send),
          InstanceMethod("getResponse", &IceServersRequest::getResponse),
        }
    );
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("IceServersRequest", func);
  return exports;
}

IceServersRequest::IceServersRequest(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<IceServersRequest>(info) {
  Napi::Env env = info.Env();

  int length = info.Length();
  if (length < 1)
  {
    Napi::TypeError::New(env, "Expects 1 argument: device").ThrowAsJavaScriptException();
  }
  Napi::Value device = info[0];

  NodeNabtoDevice* d = Napi::ObjectWrap<NodeNabtoDevice>::Unwrap(device.ToObject());
  device_ = d->getDevice();

  req_ = nabto_device_ice_servers_request_new(device_);

}

IceServersRequest::~IceServersRequest()
{
  nabto_device_ice_servers_request_free(req_);
}

Napi::Value IceServersRequest::Send(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  int length = info.Length();
  if (length < 1 || !info[0].IsString())
  {
    Napi::TypeError::New(env, "Expects 1 argument; identifier: string").ThrowAsJavaScriptException();
  }

  IceFutureContext* ifc = new IceFutureContext(device_, info[0].ToString().Utf8Value().c_str(), req_, info.Env());
  return ifc->Promise();
}

Napi::Value IceServersRequest::getResponse(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();


  size_t n = nabto_device_ice_servers_request_get_server_count(req_);
  Napi::Array resp = Napi::Array::New(env, n);

  for (size_t i = 0; i < n; i++) {
    Napi::Object serv = Napi::Object::New(env);
    const char* username = nabto_device_ice_servers_request_get_username(req_, i);
    const char* cred = nabto_device_ice_servers_request_get_credential(req_, i);
    size_t urlCount = nabto_device_ice_servers_request_get_urls_count(req_, i);
    serv.Set("username", std::string(username));
    serv.Set("credential", std::string(cred));
    Napi::Array urls = Napi::Array::New(env, urlCount);

    for (size_t u = 0; u < urlCount; u++) {
      urls[u] = std::string(nabto_device_ice_servers_request_get_url(req_, i, u));
    }
    serv.Set("Urls", urls);
    resp[i] = serv;
  }
  return resp;
}


