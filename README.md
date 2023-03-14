# embedded-sdk-node
NodeJs wrapper for the Nabto Embedded SDK


## usage

Currently tests rely on the edge-client-node repo being checkout in the same sandbox as this repo. (see Implementation Todos below)

```
cd ../
git clone git@github.com:nabto/edge-client-node.git
cd embedded-sdk-node
```

Then use
```
npm install
npm run install
npm run test
```

The `install` and `test` commands can be run as a single command with:

```
npm run buildAndTest
```

Some tests will test logging, some tests do not have logging at all, but some tests have optional logging through an environment variable:

```
NABTO_LOG_LEVEL=trace npm run buildAndTest
```


## Implementation Todos

This list contains implementation TODOs which does not fit as a comment in the code. In addition to this list, grep for `TODO:` in source.

 * Streaming
 * IAM
 * FCM
 * Service Invoke
 * Limits functions
 * Password auth
 * Include Nabto Client as submodule instead of relying on a local checkout
 * CI
 * Attached/remote tests
 * When stopping, coap/authReq listeners cause node to hang for a few sec until the garbage collector cleans it up.
