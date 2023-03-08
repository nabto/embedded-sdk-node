#include <napi.h>
#include "node_nabto_device.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return NodeNabtoDevice::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
