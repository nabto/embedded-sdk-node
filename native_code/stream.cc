#include "stream.h"
#include "node_nabto_device.h"
#include "future.h"

class AcceptFutureContext : public FutureContext
{
public:
    AcceptFutureContext(NabtoDevice* device, Napi::Env env, NabtoDeviceStream* stream) : FutureContext(device, env)
    {
        nabto_device_stream_accept(stream, future_);
        arm(false);
    }
};

class WriteFutureContext : public FutureContext
{
public:
    WriteFutureContext(NabtoDevice* device, Napi::Env env, NabtoDeviceStream* stream, Napi::ArrayBuffer data) : FutureContext(device, env)
    {
        nabto_device_stream_write(stream, future_, data.Data(), data.ByteLength());
        arm(false);
    }
};

Napi::Object StreamListener::Init(Napi::Env env, Napi::Object exports){
    Napi::Function func =
        DefineClass(
            env,
            "StreamListener",
            {
                InstanceMethod("stop", &StreamListener::Stop),
                InstanceMethod("notifyStream", &StreamListener::NotifyStream),
                InstanceMethod("getCurrentStream", &StreamListener::GetCurrentStream),
            });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("StreamListener", func);
    return exports;
}

class CloseFutureContext : public FutureContext
{
public:
    CloseFutureContext(NabtoDevice* device, Napi::Env env, NabtoDeviceStream* stream) : FutureContext(device, env)
    {
        nabto_device_stream_close(stream, future_);
        arm(false);
    }
};

StreamListener::StreamListener(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<StreamListener>(info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 2)
    {
        Napi::TypeError::New(env, "Expected 2 arguments: Device, port").ThrowAsJavaScriptException();
        return;
    }
    Napi::Value device = info[0];
    Napi::Value port = info[1];

    if (!device.IsObject()) {
        Napi::TypeError::New(env, "First arg expected Nabto Device object").ThrowAsJavaScriptException();
        return;
    }

    if (!port.IsNumber()) {
        Napi::TypeError::New(env, "Second arg expected port Number").ThrowAsJavaScriptException();
        return;
    }
    NodeNabtoDevice* d = Napi::ObjectWrap<NodeNabtoDevice>::Unwrap(device.ToObject());

    device_ = d->getDevice();

    listener_ = new StreamListenFutureContext(device_, env, port.ToNumber().Uint32Value());
}

StreamListener::~StreamListener(){

}

void StreamListener::Stop(const Napi::CallbackInfo& info){
    listener_->stop();
}


Napi::Value StreamListener::NotifyStream(const Napi::CallbackInfo& info){
    listener_->rearm();
    return listener_->Promise();
}

Napi::Value StreamListener::GetCurrentStream(const Napi::CallbackInfo& info){
    return Napi::Number::New(info.Env(), (uint64_t)listener_->getStream());
}


/**************** STREAM IMPL ***************/

Napi::Object Stream::Init(Napi::Env env, Napi::Object exports){
    Napi::Function func =
        DefineClass(
            env,
            "Stream",
            {
                InstanceMethod("accept", &Stream::Accept),
                InstanceMethod("getConnectionRef", &Stream::GetConnectionRef),
                InstanceMethod("readSome", &Stream::ReadSome),
                InstanceMethod("readAll", &Stream::ReadAll),
                InstanceMethod("write", &Stream::Write),
                InstanceMethod("close", &Stream::Close),
                InstanceMethod("abort", &Stream::Abort),
                InstanceMethod("getData", &Stream::GetData),
            });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("Stream", func);
    return exports;

}

Stream::Stream(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<Stream>(info) {
    Napi::Env env = info.Env();
    int length = info.Length();
    if (length < 2)
    {
        Napi::TypeError::New(env, "Expected 2 arguments: Device, stream").ThrowAsJavaScriptException();
        return;
    }
    Napi::Value device = info[0];
    Napi::Value stream = info[1];

    if (!device.IsObject()) {
        Napi::TypeError::New(env, "First arg expected Nabto Device object").ThrowAsJavaScriptException();
        return;
    }

    if (!stream.IsNumber()) {
        Napi::TypeError::New(env, "Second arg expected stream reference").ThrowAsJavaScriptException();
        return;
    }
    NodeNabtoDevice* d = Napi::ObjectWrap<NodeNabtoDevice>::Unwrap(device.ToObject());

    device_ = d->getDevice();
    stream_ = (NabtoDeviceStream*)stream.ToNumber().Int64Value();
}

Stream::~Stream(){

}


Napi::Value Stream::Accept(const Napi::CallbackInfo& info){
    AcceptFutureContext* afc = new AcceptFutureContext(device_, info.Env(), stream_);
    return afc->Promise();

}

Napi::Value Stream::GetConnectionRef(const Napi::CallbackInfo& info){
    NabtoDeviceConnectionRef ref = nabto_device_stream_get_connection_ref(stream_);

    return Napi::Number::New(info.Env(), (uint64_t)ref);
}

Napi::Value Stream::ReadSome(const Napi::CallbackInfo& info){
    this->reader_ = new ReadSomeFutureContext(device_, info.Env(), stream_);
    return this->reader_->Promise();
}

Napi::Value Stream::ReadAll(const Napi::CallbackInfo& info){
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected read length").ThrowAsJavaScriptException();
        return Napi::Value();
    }
    this->reader_ = new ReadAllFutureContext(device_, info.Env(), stream_, info[0].ToNumber().Uint32Value());
    return this->reader_->Promise();

}

Napi::Value Stream::Write(const Napi::CallbackInfo& info){
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1 || !info[0].IsArrayBuffer())
    {
        Napi::TypeError::New(env, "Expected arguments format: ArrayBuffer").ThrowAsJavaScriptException();
        return Napi::Value();
    }

    Napi::ArrayBuffer buf = info[0].As<Napi::ArrayBuffer>();
    WriteFutureContext* wfc = new WriteFutureContext(device_, info.Env(), stream_, buf);
    return wfc->Promise();

}

Napi::Value Stream::GetData(const Napi::CallbackInfo& info)
{
    void* payload;
    size_t length;
    // TODO: handle reader_ not set and getBuffer returning false
    this->reader_->getBuffer(&payload, &length);
    Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), payload, length);
    this->reader_ = nullptr;
    return buf;
}


Napi::Value Stream::Close(const Napi::CallbackInfo& info){
    CloseFutureContext* cfc = new CloseFutureContext(device_, info.Env(), stream_);
    return cfc->Promise();

}


void Stream::Abort(const Napi::CallbackInfo& info){
    return nabto_device_stream_abort(stream_);
}


