import 'mocha'
import chai from 'chai';

import { DeviceEvent, DeviceOptions, LogMessage, NabtoDevice, NabtoDeviceFactory } from '../src/NabtoDevice/NabtoDevice'
import { env } from 'process';

const expect = chai.expect;

const logLevel = env.NABTO_LOG_LEVEL;

let d1 = {
  productId: "pr-t3epe4p3",
  deviceId:"de-muct7xdi",
  rawKey: "37de51e06ac8d17bc3a67df1931e10a1ac232a15ab1169f965434e7f2426c086",
  pemKey: `-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIDfeUeBqyNF7w6Z98ZMeEKGsIyoVqxFp+WVDTn8kJsCGoAoGCCqGSM49
AwEHoUQDQgAET9uStjmW8mx91TiVYwyBcWgwblKWLHc9Z6bwksZGl6y21y3Cit9n
LxWbFHyckSFprZfN1MH+dLlF7ntnsmrpJw==
-----END EC PRIVATE KEY-----
`,
  fingerprint: "722b514216aaf444cbc150ac8d258d95c26558c712fab28c39de9e6cfd95aa54"
};

describe('attach tests', () => {
  let dev: NabtoDevice;
  let opts: DeviceOptions;

  beforeEach(async () => {
    dev = NabtoDeviceFactory.create();
    opts = {
      productId: d1.productId,
      deviceId: d1.deviceId,
      privateKey: d1.pemKey,
      enableMdns: true,
      localPort: 0,
      p2pPort: 0,
    }
    if (logLevel) {
      dev.setLogLevel(logLevel);
      dev.setLogCallback((logMessage: LogMessage) => {
        let now = new Date();
        if (logMessage.severity.length < 5) {
          logMessage.severity += "_"
        }
        console.log(`${now.toISOString()} [${logMessage.severity}]: ${logMessage.message}`);
      });
    }

  });

  afterEach(async () => {
    dev.stop();
  });

  it('attach pem', async () => {
    dev.setOptions(opts);
    let resolver: (value: Boolean | PromiseLike<Boolean>) => void;
    let prom = new Promise<Boolean>((res) => {
        resolver = res;
    });

    dev.onDeviceEvent(async (ev) => {
        let success = true;
        try {
        expect(ev).to.exist;
        expect(ev).to.equal(DeviceEvent.ATTACHED);
        } catch (err) {
            console.log(err);
            success = false;
        }
        resolver(success);
    });
    await dev.start();
    let res = await prom;
    expect(res).to.be.true;

  });

  it('attach raw', async () => {
    opts.privateKey = dev.createPrivateKey(); // set bad key to ensure attach do not succeed simply because setRawPrivateKey fails silently
    dev.setOptions(opts);
    dev.experimental.setRawPrivateKey(d1.rawKey);
    let resolver: (value: Boolean | PromiseLike<Boolean>) => void;
    let prom = new Promise<Boolean>((res) => {
        resolver = res;
    });

    dev.onDeviceEvent(async (ev) => {
        let success = true;
        try {
        expect(ev).to.exist;
        expect(ev).to.equal(DeviceEvent.ATTACHED);
        } catch (err) {
            console.log(err);
            success = false;
        }
        resolver(success);
    });
    await dev.start();
    let res = await prom;
    expect(res).to.be.true;

  });

});
