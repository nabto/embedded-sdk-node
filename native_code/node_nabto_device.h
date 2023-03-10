#pragma once

#include <napi.h>
#include <nabto/nabto_device.h>
#include "future.h"
#include "connection_events.h"

class LogMessage {
 public:
    LogMessage(std::string message, std::string severity)
      : message_(message), severity_(severity)
    {
    }

    std::string message_;
    std::string severity_;
};

class DeviceEventFutureContext: public FutureContext
{
  public:
  DeviceEventFutureContext(NabtoDevice* device, Napi::Env env) : FutureContext(device, env)
  {
    lis_ = nabto_device_listener_new(device_);
    NabtoDeviceError ec = nabto_device_device_events_init_listener(device_, lis_);
    // TODO: error handling

    nabto_device_listener_device_event(lis_, future_, &event_);
    arm(true);
  }

  void rearm() {
    nabto_device_listener_device_event(lis_, future_, &event_);
    arm(true);
  }

// TODO: replace this with new embedded sdk api function for this.
  std::string getEventString() {
    if (event_ == NABTO_DEVICE_EVENT_ATTACHED) {
        return "NABTO_DEVICE_EVENT_ATTACHED";
    } else if (event_ == NABTO_DEVICE_EVENT_DETACHED) {
        return "NABTO_DEVICE_EVENT_DETACHED";
    } else if (event_ == NABTO_DEVICE_EVENT_CLOSED) {
        return "NABTO_DEVICE_EVENT_CLOSED";
    } else if (event_ == NABTO_DEVICE_EVENT_UNKNOWN_FINGERPRINT) {
        return "NABTO_DEVICE_EVENT_UNKNOWN_FINGERPRINT";
    } else if (event_ == NABTO_DEVICE_EVENT_WRONG_PRODUCT_ID) {
        return "NABTO_DEVICE_EVENT_WRONG_PRODUCT_ID";
    } else if (event_ == NABTO_DEVICE_EVENT_WRONG_DEVICE_ID) {
        return "NABTO_DEVICE_EVENT_WRONG_DEVICE_ID";
    } else {
        return "";
    }
  }

  NabtoDeviceListener* lis_;
  NabtoDeviceEvent event_;
};

using Context = void;
void CallJs(Napi::Env env, Napi::Function callback, Context *context, LogMessage *data);
typedef Napi::TypedThreadSafeFunction<Context, LogMessage, CallJs> LogCallbackFunction;


class NodeNabtoDevice : public Napi::ObjectWrap<NodeNabtoDevice> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  virtual void Finalize(Napi::Env env);
  NodeNabtoDevice(const Napi::CallbackInfo& info);
  ~NodeNabtoDevice();
  void Stop(const Napi::CallbackInfo& info);
  Napi::Value Start(const Napi::CallbackInfo& info);

  NabtoDevice* getDevice() { return nabtoDevice_; }

 private:
  static void LogCallback(NabtoDeviceLogMessage* log, void* userData);

  Napi::Value GetVersion(const Napi::CallbackInfo& info);
  void SetOptions(const Napi::CallbackInfo& info);
  Napi::Value CreatePrivateKey(const Napi::CallbackInfo& info);
  void SetLogLevel(const Napi::CallbackInfo& info);
  void SetLogCallback(const Napi::CallbackInfo& info);
  Napi::Value GetConfiguration(const Napi::CallbackInfo& info);
  void SetBasestationAttach(const Napi::CallbackInfo& info);
  Napi::Value NotifyDeviceEvent(const Napi::CallbackInfo& info);
  Napi::Value GetCurrentDeviceEvent(const Napi::CallbackInfo& info);
  Napi::Value NotifyConnectionEvent(const Napi::CallbackInfo& info);
  Napi::Value GetCurrentConnectionEvent(const Napi::CallbackInfo& info);
  Napi::Value GetCurrentConnectionRef(const Napi::CallbackInfo& info);

  NabtoDevice* nabtoDevice_;
  LogCallbackFunction logCallback_;
  DeviceEventFutureContext* devEvents_;
  ConnectionEventFutureContext* connEvents_;
};
