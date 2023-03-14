#include "coap.h"
#include "node_nabto_device.h"
#include "future.h"


Napi::Object CoapEndpoint::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func =
        DefineClass(
            env,
            "CoapEndpoint",
            {
                InstanceMethod("stop", &CoapEndpoint::Stop),
                InstanceMethod("notifyRequest", &CoapEndpoint::NotifyRequest),
                InstanceMethod("getCurrentRequest", &CoapEndpoint::GetCurrentRequest),
            });

    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("CoapEndpoint", func);
    return exports;
}

CoapEndpoint::CoapEndpoint(const Napi::CallbackInfo &info)
: Napi::ObjectWrap<CoapEndpoint>(info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 3)
    {
        Napi::TypeError::New(env, "Expected 3 arguments: Device, method, path").ThrowAsJavaScriptException();
        return;
    }
    Napi::Value device = info[0];
    Napi::Value method = info[1];
    Napi::Value path = info[2];

    if(!device.IsObject()) {
         Napi::TypeError::New(env, "First arg expected Nabto Device object").ThrowAsJavaScriptException();
        return;
    }

    if(!method.IsString()) {
         Napi::TypeError::New(env, "Second arg expected method string").ThrowAsJavaScriptException();
        return;
    }

    if(!path.IsString()) {
         Napi::TypeError::New(env, "Third arg expected path string").ThrowAsJavaScriptException();
        return;
    }

    NodeNabtoDevice *d = Napi::ObjectWrap<NodeNabtoDevice>::Unwrap(device.ToObject());

    device_ = d->getDevice();

    listener_ = new CoapRequestFutureContext(device_, env, method.ToString().Utf8Value(), path.ToString().Utf8Value());
}

CoapEndpoint::~CoapEndpoint()
{
}

void CoapEndpoint::Stop(const Napi::CallbackInfo &info)
{
    listener_->stop();
}

Napi::Value CoapEndpoint::NotifyRequest(const Napi::CallbackInfo &info)
{
    listener_->rearm();
    return listener_->Promise();
}

Napi::Value CoapEndpoint::GetCurrentRequest(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), (uint64_t)listener_->getRequest());

}




/******************* COAP REQUEST IMPL ************/

Napi::Object CoapRequest::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func =
        DefineClass(
            env,
            "CoapRequest",
            {
                InstanceMethod("getFormat", &CoapRequest::GetFormat),
                InstanceMethod("getPayload", &CoapRequest::GetPayload),
                InstanceMethod("getConnectionRef", &CoapRequest::GetConnectionRef),
                InstanceMethod("getParameter", &CoapRequest::GetParameter),
                InstanceMethod("sendErrorResponse", &CoapRequest::SendErrorResponse),
                InstanceMethod("setResponseCode", &CoapRequest::SetResponseCode),
                InstanceMethod("setResponsePayload", &CoapRequest::SetResponsePayload),
                InstanceMethod("responseReady", &CoapRequest::ResponseReady),
            });

    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("CoapRequest", func);
    return exports;
}

CoapRequest::CoapRequest(const Napi::CallbackInfo &info)
: Napi::ObjectWrap<CoapRequest>(info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected coapRequst reference").ThrowAsJavaScriptException();
        return;
    }
    req_ = (NabtoDeviceCoapRequest*)info[0].ToNumber().Int64Value();
}

CoapRequest::~CoapRequest()
{
}

Napi::Value CoapRequest::GetFormat(const Napi::CallbackInfo &info)
{
    uint16_t format;
    NabtoDeviceError ec = nabto_device_coap_request_get_content_format(req_, &format);
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return Napi::Value();
    }
    return Napi::Number::New(info.Env(), format);
}

Napi::Value CoapRequest::GetPayload(const Napi::CallbackInfo &info)
{
    void* payload;
    size_t length;
    NabtoDeviceError ec = nabto_device_coap_request_get_payload(req_, &payload, &length);
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return Napi::Value();
    }
    Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), payload, length);
    return buf;
}

Napi::Value CoapRequest::GetConnectionRef(const Napi::CallbackInfo &info)
{
    NabtoDeviceConnectionRef ref = nabto_device_coap_request_get_connection_ref(req_);

    return Napi::Number::New(info.Env(), (uint64_t)ref);
}

Napi::Value CoapRequest::GetParameter(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected String argument").ThrowAsJavaScriptException();
        return Napi::Value();
    }
    const char* param = nabto_device_coap_request_get_parameter(req_, info[0].ToString().Utf8Value().c_str());
    return Napi::String::New(env, param);
}

void CoapRequest::SendErrorResponse(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 2 || !info[0].IsNumber() || !info[1].IsString())
    {
        Napi::TypeError::New(env, "Expected arguments code: Number, message: String").ThrowAsJavaScriptException();
        return;
    }

    NabtoDeviceError ec = nabto_device_coap_error_response(req_, info[0].ToNumber().Uint32Value(), info[1].ToString().Utf8Value().c_str());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
}

void CoapRequest::SetResponseCode(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected argument code: Number").ThrowAsJavaScriptException();
        return;
    }

    NabtoDeviceError ec = nabto_device_coap_response_set_code(req_, info[0].ToNumber().Uint32Value());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
}

void CoapRequest::SetResponsePayload(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length < 2 || !info[0].IsNumber() || !info[1].IsArrayBuffer())
    {
        Napi::TypeError::New(env, "Expected arguments format: Number, message: ArrayBuffer").ThrowAsJavaScriptException();
        return;
    }

    Napi::Number contentFormat = info[0].ToNumber();
    Napi::ArrayBuffer buf = info[1].As<Napi::ArrayBuffer>();

    NabtoDeviceError ec = nabto_device_coap_response_set_payload(req_, buf.Data(), buf.ByteLength());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
    ec = nabto_device_coap_response_set_content_format(req_, contentFormat.Uint32Value());
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
}

void CoapRequest::ResponseReady(const Napi::CallbackInfo &info)
{
    NabtoDeviceError ec = nabto_device_coap_response_ready(req_);
    if (ec != NABTO_DEVICE_EC_OK) {
        Napi::TypeError::New(info.Env(), nabto_device_error_get_message(ec)).ThrowAsJavaScriptException();
        return;
    }
}
