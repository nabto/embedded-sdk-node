#pragma once

#include <napi.h>

#include "future.h"


class ConnectionEventFutureContext: public FutureContext
{
  public:
  ConnectionEventFutureContext(NabtoDevice* device, Napi::Env env) : FutureContext(device, env)
  {
    lis_ = nabto_device_listener_new(device_);
    NabtoDeviceError ec = nabto_device_connection_events_init_listener(device_, lis_);
    if (ec != NABTO_DEVICE_EC_OK) {
      // TODO: error handling
    }

    nabto_device_listener_connection_event(lis_, future_, &ref_, &event_);
    arm(true);
  }

  ~ConnectionEventFutureContext() {
    nabto_device_listener_free(lis_);
  }

  void rearm() {
    nabto_device_listener_connection_event(lis_, future_, &ref_, &event_);
    arm(true);
  }

// TODO: replace this with new embedded sdk api function for this.
  std::string getEventString() {
    if (event_ == NABTO_DEVICE_CONNECTION_EVENT_OPENED) {
        return "NABTO_DEVICE_CONNECTION_EVENT_OPENED";
    } else if (event_ == NABTO_DEVICE_CONNECTION_EVENT_CLOSED) {
        return "NABTO_DEVICE_CONNECTION_EVENT_CLOSED";
    } else if (event_ == NABTO_DEVICE_CONNECTION_EVENT_CHANNEL_CHANGED) {
        return "NABTO_DEVICE_CONNECTION_EVENT_CHANNEL_CHANGED";
    } else {
        return "";
    }
  }

  uint64_t getConnectionRef() {
    return ref_;
  }

  NabtoDeviceListener* lis_;
  NabtoDeviceConnectionEvent event_;
  NabtoDeviceConnectionRef ref_;
};

