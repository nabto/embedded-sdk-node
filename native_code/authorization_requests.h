#pragma once

#include <napi.h>
#include "future.h"

class AuthRequestFutureContext : public FutureContext
{
public:
    AuthRequestFutureContext(NabtoDevice *device, Napi::Env env) : FutureContext(device, env)
    {
        lis_ = nabto_device_listener_new(device_);

        NabtoDeviceError ec = nabto_device_authorization_request_init_listener(device_, lis_);
        if (ec != NABTO_DEVICE_EC_OK)
        {
            // TODO: error handling
        }
    }

    ~AuthRequestFutureContext()
    {
        nabto_device_listener_free(lis_);
    }
    void rearm()
    {
        nabto_device_listener_new_authorization_request(lis_, future_, &req_);
        arm(true);
    }

    NabtoDeviceAuthorizationRequest *getRequest()
    {
        return req_;
    }

private:
    NabtoDeviceListener *lis_;
    NabtoDeviceAuthorizationRequest *req_;
};

class AuthHandler : public Napi::ObjectWrap<AuthHandler>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func =
            DefineClass(
                env,
                "AuthHandler",
                {
                    InstanceMethod("stop", &AuthHandler::Stop),
                    InstanceMethod("notifyRequest", &AuthHandler::NotifyRequest),
                    InstanceMethod("getCurrentRequest", &AuthHandler::GetCurrentRequest),
                });

        Napi::FunctionReference *constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("AuthHandler", func);
        return exports;
    }

    AuthHandler(const Napi::CallbackInfo &info)
        : Napi::ObjectWrap<AuthHandler>(info)
    {
        Napi::Env env = info.Env();

        int length = info.Length();
        if (length < 1)
        {
            Napi::TypeError::New(env, "Expected 1 argument: device").ThrowAsJavaScriptException();
            return;
        }
        Napi::Value device = info[0];
        if (!device.IsObject())
        {
            Napi::TypeError::New(env, "First arg expected Nabto Device object").ThrowAsJavaScriptException();
            return;
        }
        NodeNabtoDevice *d = Napi::ObjectWrap<NodeNabtoDevice>::Unwrap(device.ToObject());

        device_ = d->getDevice();

        listener_ = new AuthRequestFutureContext(device_, env);
    }

    ~AuthHandler() {}

    void Stop(const Napi::CallbackInfo &info)
    {
        listener_->stop();
    }

    Napi::Value NotifyRequest(const Napi::CallbackInfo &info)
    {
        listener_->rearm();
        return listener_->Promise();
    }

    Napi::Value GetCurrentRequest(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), (uint64_t)listener_->getRequest());
    }

private:
    NabtoDevice *device_;
    AuthRequestFutureContext *listener_;
};

class AuthRequest : public Napi::ObjectWrap<AuthRequest>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func =
            DefineClass(
                env,
                "AuthRequest",
                {
                    InstanceMethod("verdict", &AuthRequest::Verdict),
                    InstanceMethod("getAction", &AuthRequest::GetAction),
                    InstanceMethod("getConnectionRef", &AuthRequest::GetConnectionRef),
                    InstanceMethod("getAttributes", &AuthRequest::GetAttributes),
                });

        Napi::FunctionReference *constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("AuthRequest", func);
        return exports;
    }

    AuthRequest(const Napi::CallbackInfo &info)
        : Napi::ObjectWrap<AuthRequest>(info)
    {
        Napi::Env env = info.Env();

        int length = info.Length();
        if (length < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Expected AuthorizationRequst reference").ThrowAsJavaScriptException();
            return;
        }
        req_ = (NabtoDeviceAuthorizationRequest *)info[0].ToNumber().Int64Value();
    }

    ~AuthRequest() {}

    void Verdict(const Napi::CallbackInfo &info)
    {
        int length = info.Length();
        if (length <= 0 || !info[0].IsBoolean())
        {
            Napi::TypeError::New(info.Env(), "Boolean expected").ThrowAsJavaScriptException();
            return;
        }

        nabto_device_authorization_request_verdict(req_, info[0].ToBoolean().Value());
    }

    Napi::Value GetAction(const Napi::CallbackInfo &info)
    {

        const char *action = nabto_device_authorization_request_get_action(req_);
        return Napi::String::New(info.Env(), action);
    }

    Napi::Value GetConnectionRef(const Napi::CallbackInfo &info)
    {
        NabtoDeviceConnectionRef ref = nabto_device_authorization_request_get_connection_ref(req_);

        return Napi::Number::New(info.Env(), (uint64_t)ref);
    }

    Napi::Value GetAttributes(const Napi::CallbackInfo &info)
    {
        size_t size = nabto_device_authorization_request_get_attributes_size(req_);
        Napi::Object retVal = Napi::Object::New(info.Env());
        for (size_t i = 0; i < size; i++) {
            const char* name = nabto_device_authorization_request_get_attribute_name(req_, i);
            const char* value = nabto_device_authorization_request_get_attribute_value(req_, i);
            retVal.Set(name, value);
        }
        return retVal;
    }

private:
    NabtoDeviceAuthorizationRequest *req_;
};
