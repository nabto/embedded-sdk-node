import 'mocha'
import { strict as assert } from 'node:assert';
import chai from 'chai';

import { DeviceOptions, LogMessage, NabtoDevice } from '../src/NabtoDevice/NabtoDevice'

const expect = chai.expect;


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
        console.log(logMessage);
    });
    await dev.start();
  })

});
