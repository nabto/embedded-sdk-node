#pragma once

#include <napi.h>
#include <nabto/nabto_device.h>

class FutureContext
{
public:
    FutureContext(NabtoDevice *device, Napi::Env env)
    : future_(nabto_device_future_new(device)), device_(device), deferred_(Napi::Promise::Deferred::New(env))
    {
        ttsf_ = TTSF::New(env, "TSFN", 0, 1, this, [](Napi::Env, void *, FutureContext *ctx)
                          { delete ctx; });
    }

    virtual ~FutureContext()
    {
        nabto_device_future_free(future_);
    }

    void arm(bool repeatable) {
        repeatable_ = repeatable;
        deferred_ = Napi::Promise::Deferred::New(deferred_.Env());
        nabto_device_future_set_callback(future_, FutureContext::futureCallback, this);
    }

    void stop() {
        ttsf_.Release();
    }

    static void CallJS(Napi::Env env, Napi::Function callback, FutureContext *context, void **data)
    {
        if (context->ec_ == NABTO_DEVICE_EC_OK)
        {
            context->deferred_.Resolve(env.Undefined());
        }
        else
        {
            context->deferred_.Reject(Napi::Error::New(env, nabto_device_error_get_message(context->ec_)).Value());
        }
    }
    typedef Napi::TypedThreadSafeFunction<FutureContext, void *, FutureContext::CallJS> TTSF;

    static void futureCallback(NabtoDeviceFuture *future, NabtoDeviceError ec, void *userData)
    {
        auto ctx = static_cast<FutureContext *>(userData);
        ctx->ec_ = ec;

        ctx->ttsf_.NonBlockingCall();
        if (!ctx->repeatable_) {
            ctx->stop();
        }
    }

    Napi::Value Promise()
    {
        return deferred_.Promise();
    }

    NabtoDeviceFuture* future_;
    NabtoDevice* device_;
    TTSF ttsf_;
    Napi::Promise::Deferred deferred_;
    NabtoDeviceError ec_;
    bool repeatable_ = false;
};
