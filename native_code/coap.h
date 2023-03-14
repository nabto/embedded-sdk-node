#pragma once

#include <napi.h>
#include "future.h"

class CoapRequestFutureContext : public FutureContext
{
public:
    CoapRequestFutureContext(NabtoDevice *device, Napi::Env env, std::string method, std::string path) : FutureContext(device, env)
    {
        lis_ = nabto_device_listener_new(device_);

        auto m = methodFromString(method);
        createPath(path);

        const char** segments = (const char**)calloc(path_.size()+1, sizeof(const char*));
        size_t i = 0;
        for (i = 0; i < path_.size(); i++) {
            segments[i] = path_[i].c_str();
        }
        segments[i] = NULL;

        NabtoDeviceError ec = nabto_device_coap_init_listener(device_, lis_, m, segments);
        if (ec != NABTO_DEVICE_EC_OK) {
            // TODO: error handling
        }
        free(segments);
    }

    ~CoapRequestFutureContext() {
        nabto_device_listener_free(lis_);
    }
    void rearm()
    {
        nabto_device_listener_new_coap_request(lis_, future_, &req_);
        arm(true);
    }

    NabtoDeviceCoapRequest* getRequest()
    {
        return req_;
    }

private:
    NabtoDeviceCoapMethod methodFromString(std::string method)
    {
        if (method == "NABTO_DEVICE_COAP_GET")
        {
            return NABTO_DEVICE_COAP_GET;
        }
        else if (method == "NABTO_DEVICE_COAP_POST")
        {
            return NABTO_DEVICE_COAP_POST;
        }
        else if (method == "NABTO_DEVICE_COAP_PUT")
        {
            return NABTO_DEVICE_COAP_PUT;
        }
        else if (method == "NABTO_DEVICE_COAP_DELETE")
        {
            return NABTO_DEVICE_COAP_DELETE;
        }
        else
        {
            return NABTO_DEVICE_COAP_GET;
        }
    }

    void createPath(std::string path)
    {
        size_t begin = 0;
        path_.clear();
        pathCStr_.clear();
        size_t i = 0;
        for (i = 0; i < path.size(); i++) {
            if (path[i] == '/') {
                if (i == begin) {
                    begin++;
                    continue;
                }
                std::string seg = path.substr(begin, i-begin);
                path_.push_back(seg);
                pathCStr_.push_back(seg.c_str());
                begin = i + 1;
            }
        }
        if (begin < i) { // we did not end on a /
            std::string seg = path.substr(begin, i-begin);
            path_.push_back(seg);
            pathCStr_.push_back(seg.c_str());
        }
        pathCStr_.push_back(NULL);
    }

//    char** path_;
    std::vector<std::string> path_;
    std::vector<const char*> pathCStr_;

    NabtoDeviceListener *lis_;
    NabtoDeviceCoapRequest* req_;
};


class CoapEndpoint : public Napi::ObjectWrap<CoapEndpoint>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    CoapEndpoint(const Napi::CallbackInfo &info);
    ~CoapEndpoint();
    void Stop(const Napi::CallbackInfo &info);

    Napi::Value NotifyRequest(const Napi::CallbackInfo &info);
    Napi::Value GetCurrentRequest(const Napi::CallbackInfo &info);

private:
    NabtoDevice* device_;
    CoapRequestFutureContext* listener_;
};

class CoapRequest : public Napi::ObjectWrap<CoapRequest>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    CoapRequest(const Napi::CallbackInfo &info);
    ~CoapRequest();

    Napi::Value GetFormat(const Napi::CallbackInfo &info);
    Napi::Value GetPayload(const Napi::CallbackInfo &info);
    Napi::Value GetConnectionRef(const Napi::CallbackInfo &info);
    Napi::Value GetParameter(const Napi::CallbackInfo &info);

    void SendErrorResponse(const Napi::CallbackInfo &info);
    void SetResponseCode(const Napi::CallbackInfo &info);
    void SetResponsePayload(const Napi::CallbackInfo &info);
    void ResponseReady(const Napi::CallbackInfo &info);


private:
    NabtoDeviceCoapRequest* req_;
};
