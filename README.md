# embedded-sdk-node
NodeJs wrapper for the Nabto Embedded SDK


## usage

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
