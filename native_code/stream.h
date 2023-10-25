#pragma once

#include <napi.h>
#include "future.h"

class StreamListenFutureContext : public FutureContext
{
public:
    StreamListenFutureContext(NabtoDevice* device, Napi::Env env, uint32_t port) : FutureContext(device, env)
    {
        lis_ = nabto_device_listener_new(device_);
        NabtoDeviceError ec;

        if (port == 0) {
            ec = nabto_device_stream_init_listener_ephemeral(device_, lis_, &port_);
        } else {
            port_ = port;
            ec = nabto_device_stream_init_listener(device_, lis_, port_);
        }
        if (ec != NABTO_DEVICE_EC_OK) {
            // TODO: error handling
        }
    }

    ~StreamListenFutureContext() {
        nabto_device_listener_free(lis_);
    }
    void rearm()
    {
        nabto_device_listener_new_stream(lis_, future_, &stream_);
        arm(true);
    }

    NabtoDeviceStream* getStream()
    {
        return stream_;
    }

    uint32_t getPort()
    {
        return port_;
    }

private:

    NabtoDeviceListener* lis_;
    NabtoDeviceStream* stream_;
    uint32_t port_;
};

class ReadFutureContext : public FutureContext
{
public:
    ReadFutureContext(NabtoDevice* device, Napi::Env env, NabtoDeviceStream* stream) : FutureContext(device, env)
    {
        stream_ = stream;
    }

    ~ReadFutureContext()
    {
        free(readBuffer_);
    }

    bool getBuffer(void** buf, size_t* len)
    {
        *buf = readBuffer_;
        *len = readLength_;
        return true;
    }

protected:
    NabtoDeviceStream* stream_;
    void* readBuffer_;
    size_t readLength_;
};

class ReadSomeFutureContext : public ReadFutureContext
{
public:
    ReadSomeFutureContext(NabtoDevice* device, Napi::Env env, NabtoDeviceStream* stream) : ReadFutureContext(device, env, stream)
    {
        readBuffer_ = calloc(1, 1024);
        nabto_device_stream_read_some(stream, future_, (void*)readBuffer_, 1024, &readLength_);
        arm(false);
    }
};

class ReadAllFutureContext : public ReadFutureContext
{
public:
    ReadAllFutureContext(NabtoDevice* device, Napi::Env env, NabtoDeviceStream* stream, size_t length) : ReadFutureContext(device, env, stream)
    {
        readBuffer_ = calloc(1, length);
        nabto_device_stream_read_all(stream, future_, (void*)readBuffer_, length, &readLength_);
        arm(false);
    }
};



class StreamListener : public Napi::ObjectWrap<StreamListener>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    StreamListener(const Napi::CallbackInfo& info);
    ~StreamListener();
    void Stop(const Napi::CallbackInfo& info);

    Napi::Value NotifyStream(const Napi::CallbackInfo& info);
    Napi::Value GetCurrentStream(const Napi::CallbackInfo& info);
    Napi::Value GetStreamPort(const Napi::CallbackInfo& info);

private:
    NabtoDevice* device_;
    StreamListenFutureContext* listener_;
    uint32_t port_;
};

class Stream : public Napi::ObjectWrap<Stream>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    Stream(const Napi::CallbackInfo& info);
    ~Stream();

    Napi::Value Accept(const Napi::CallbackInfo& info);
    Napi::Value GetConnectionRef(const Napi::CallbackInfo& info);
    Napi::Value ReadSome(const Napi::CallbackInfo& info);
    Napi::Value ReadAll(const Napi::CallbackInfo& info);
    Napi::Value Write(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);

    void Abort(const Napi::CallbackInfo& info);
    Napi::Value GetData(const Napi::CallbackInfo& info);


private:
    NabtoDevice* device_;
    NabtoDeviceStream* stream_;
    ReadFutureContext* reader_;
};
