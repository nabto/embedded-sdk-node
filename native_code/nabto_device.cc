#include <napi.h>
#include "node_nabto_device.h"
#include "coap.h"
#include "stream.h"
#include "authorization_requests.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  Napi::Object tmp = NodeNabtoDevice::Init(env, exports);
  tmp = CoapRequest::Init(env, exports);
  tmp = CoapEndpoint::Init(env, exports);
  tmp = Stream::Init(env, exports);
  tmp = StreamListener::Init(env, exports);
  tmp = AuthRequest::Init(env, exports);
  tmp = AuthHandler::Init(env, exports);
  tmp = IceServersRequest::Init(env, exports);
  return tmp;
}

NODE_API_MODULE(addon, InitAll)
