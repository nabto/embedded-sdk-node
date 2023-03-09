import 'mocha'
import { strict as assert } from 'node:assert';
import chai from 'chai';

import { DeviceOptions, LogMessage, NabtoDevice } from '../src/NabtoDevice/NabtoDevice'

const expect = chai.expect;

// certificate-authority/test/test-root/certs/test_root.crt
const testRootCa = `(-----BEGIN CERTIFICATE-----
MIIBpTCCAUqgAwIBAgIUev5miQGPmjlHmisQJ5iYiq+Lf0kwCgYIKoZIzj0EAwIw
MDELMAkGA1UEBhMCREsxDTALBgNVBAoMBFRlc3QxEjAQBgNVBAMMCVRlc3QgUm9v
dDAeFw0yMDA2MjAwMDAwMDBaFw00OTEyMzEyMzU5NTlaMDAxCzAJBgNVBAYTAkRL
MQ0wCwYDVQQKDARUZXN0MRIwEAYDVQQDDAlUZXN0IFJvb3QwWTATBgcqhkjOPQIB
BggqhkjOPQMBBwNCAATWs9bVLhO8o+42UrDFZocbMjvt20ODDwjxjC5/lSKo8KU6
yPcBsI6IMg+CfMfQpza7V5m9c/mHXw1r8iiOrizio0IwQDAdBgNVHQ4EFgQUcqIP
gyPbvPkv4JCNZ/Al3yXTvI4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwCgYIKoZIzj0EAwIDSQAwRgIhAKjXktlBZjxURdyDvlvPUn73cNz8MOTs7wl3
ogsvei0AAiEA4r6s8iI6b37agG6zXsPKXwjTw3jS4acs1feiZ4Vo1NE=
-----END CERTIFICATE-----
`;

describe('lifecycle', () => {
  let dev: NabtoDevice;

  beforeEach(() => {
    dev = new NabtoDevice();
  });

  afterEach(() => {
    dev.stop();
  });


  it('test get version', () => {
    let version = dev.version();
    expect(version).to.exist;
    expect(version).to.be.a("string");
  });

  it('set undefined options', () => {
    try {
        dev.setOptions(<DeviceOptions><unknown>undefined);
        expect(true).to.be.false;
    } catch (err) {
        expect(err).to.exist;
    }
  });

  it('set empty options', () => {
    try {
        dev.setOptions(<DeviceOptions>{});
        expect(true).to.be.false;
    } catch (err) {
        expect(err).to.exist;
    }
  });

  it('set invalid options', () => {
    try {
        dev.setOptions(<DeviceOptions><unknown>{ productId: 54, deviceId: true, privateKey: ["32", 24]});
        expect(true).to.be.false;
    } catch (err) {
        expect(err).to.exist;
    }
  });

  it('set bad key options', () => {
    let opts: DeviceOptions = {
        productId: "pr-foobar",
        deviceId: "de-foobar",
        privateKey: "foobar"
    }
    try {
        dev.setOptions(opts);
        expect(true).to.be.false;
    } catch (err) {
        expect(err).to.exist;
    }
  });

  it('create/set private key', () => {
    let key = dev.createPrivateKey();
    expect(key).to.be.a("string");
    let opts: DeviceOptions = {
        productId: "pr-foobar",
        deviceId: "de-foobar",
        privateKey: key
    }
    dev.setOptions(opts);
  });

  it('set options', () => {
    let key = dev.createPrivateKey();
    expect(key).to.be.a("string");
    let opts: DeviceOptions = {
        productId: "pr-foobar",
        deviceId: "de-foobar",
        privateKey: key,
        serverUrl: "foo.bar.dev.nabto.net",
        serverPort: 4242,
        rootCerts: testRootCa,
        appName: "my app",
        appVersion: "0.0.42",
        localPort: 5252,
        p2pPort: 6363,
    }
    dev.setOptions(opts);

    let conf = dev.getConfiguration();
    expect(conf).to.exist;
    expect(conf.productId).to.exist.and.be.a("string").that.equals(opts.productId);
    expect(conf.deviceId).to.exist.and.be.a("string").that.equals(opts.deviceId);
    expect(conf.appName).to.exist.and.be.a("string").that.equals(opts.appName);
    expect(conf.appVersion).to.exist.and.be.a("string").that.equals(opts.appVersion);
    // local/p2p ports are 0 until device is started
    expect(conf.localPort).to.exist.and.be.a("number").that.equals(0);
    expect(conf.p2pPort).to.exist.and.be.a("number").that.equals(0);
    expect(conf.deviceFingerprint).to.exist.and.be.a("string").that.has.length(64);
  });

  it('setup logging', () => {
    dev.setLogLevel("trace");
    dev.setLogCallback((logMessage: LogMessage) => {
        console.log(logMessage);
    });
  });

  it('start device', async () => {
    let key = dev.createPrivateKey();
    expect(key).to.be.a("string");
    let opts: DeviceOptions = {
        productId: "pr-foobar",
        deviceId: "de-foobar",
        privateKey: key
    }
    dev.setOptions(opts);
    dev.setLogLevel("trace");
    dev.setLogCallback((logMessage: LogMessage) => {
        let now = new Date();
        if (logMessage.severity.length < 5) {
            logMessage.severity += "_"
        }
        console.log(`${now.toISOString()} [${logMessage.severity}]: ${logMessage.message}`);
    });
    await dev.start();
  })

});
