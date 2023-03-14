#include <napi.h>
#include "node_nabto_device.h"
#include "coap.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  Napi::Object tmp = NodeNabtoDevice::Init(env, exports);
  tmp = CoapRequest::Init(env, exports);
  return tmp = CoapEndpoint::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
