import 'mocha'
import chai from 'chai';
import { ConnectionEvent, DeviceOptions, LogMessage, NabtoDevice, NabtoDeviceFactory } from '../src/NabtoDevice/NabtoDevice';
import { env } from 'process';
import { Connection, NabtoClient, NabtoClientFactory } from '../../edge-client-node/src/NabtoClient/NabtoClient'

const expect = chai.expect;

const logLevel = env.NABTO_LOG_LEVEL;

describe.only('connect local', () => {
  let dev: NabtoDevice;
  let cli: NabtoClient | undefined;
  let conn: Connection | undefined;

  beforeEach(async () => {
    dev = NabtoDeviceFactory.create();
    let key = dev.createPrivateKey();
    expect(key).to.be.a("string");
    let opts: DeviceOptions = {
        productId: "pr-foobar",
        deviceId: "de-foobar",
        privateKey: key,
        enableMdns: true,
        localPort: 0,
        p2pPort: 0,
    }
    dev.setOptions(opts);
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
    // If expect fails in a test, the remaining code is not executed, so to keep tests from hanging we must close connections from here
    if (conn) {
      await conn.close();
      conn = undefined;
    }
    dev.stop();
    if (cli) {
      cli.stop();
      cli = undefined;
    }
  });

  it('connect', async () => {
    await dev.start();

    let cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
  });

  it('connection events', async() => {
    let resolver: (value: Boolean | PromiseLike<Boolean>) => void;
    let prom = new Promise<Boolean>((res) => {
      resolver = res;
    });
    let resolver2: (value: Boolean | PromiseLike<Boolean>) => void;
    let prom2 = new Promise<Boolean>((res) => {
      resolver2 = res;
    });
    let count = 0;
    let connRef;

    dev.onConnectionEvent(async (ev, ref) => {
      let success = true;
      count++;
      if (count == 1) {
        try {
          expect(ev).to.exist;
          expect(ev).to.equal(ConnectionEvent.OPENED);
          expect(ref).to.exist;
          connRef = ref;
        } catch (err) {
          console.log(err);
          success = false;
        }
        resolver(success);
      } else if (count == 2) {
        try {
          expect(ev).to.exist;
          expect(ev).to.equal(ConnectionEvent.CLOSED);
          expect(ref).to.exist;
        } catch (err) {
          console.log(err);
          success = false;
        }
        resolver2(success);

      } else {
        console.log("Unexpected connection event: ", ev);
      }

    });
    await dev.start();

    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
    let res = await prom;
    expect(res).to.be.true;
    expect(connRef).to.exist;

    let cliFp = conn.getClientFingerprint();
    let cliFp2 = dev.connection.getClientFingerprint(connRef);
    expect(cliFp2).to.equal(cliFp);

    await conn.close();
    let res2 = await prom2;
    expect(res2).to.be.true;
  });


});
