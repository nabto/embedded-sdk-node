import 'mocha'
import chai from 'chai';
import { CoapMethod, CoapRequest, ConnectionEvent, DeviceOptions, LogMessage, NabtoDevice, NabtoDeviceFactory } from '../src/NabtoDevice/NabtoDevice';
import { env } from 'process';
import { Connection, NabtoClient, NabtoClientFactory } from 'edge-client-node'

const expect = chai.expect;

const logLevel = env.NABTO_LOG_LEVEL;

describe('connect local', () => {
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
      // TODO: If conn.close() is not run, cli.stop() should close the connection, and if not, dev.stop() should also do it. However, cli.stop() runs but does not seem to close the connection, and then dev.stop() simply hangs until the test timeout is reached.
      try {
        await conn.close();
      } catch(err) { } // We might already have closed if testing closure
      conn = undefined;
    }
    if (cli) {
      cli.stop();
      cli = undefined;
    }
    dev.stop();
  });

  it('connect', async () => {
    await dev.start();

    cli = NabtoClientFactory.create();
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

    let isLocal = dev.connection.isLocal(connRef);
    expect(isLocal).to.be.a("Boolean");
    expect(isLocal).to.be.true;

    await conn.close();
    let res2 = await prom2;
    expect(res2).to.be.true;
  });

  it('mdns subtype and txt items', async () => {
    dev.mdnsAddSubtype("testtype");
    dev.mdnsAddTxtItem("foo", "bar");
    await dev.start();

    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
    // TODO: test mDNS from once implemented in client
  });

  // TODO: test connection.isPasswordAuth and connection.getPasswordAuthUsername when implemented in client

  it('register coap ep', async () => {
    dev.addCoapEndpoint(CoapMethod.GET, '/hello/world', (req: CoapRequest) => {

    });
    await dev.start();
  });

  it('get coap request', async () => {
    let data = "Hello world";
    let path = '/hello/world';
    dev.addCoapEndpoint(CoapMethod.GET, path, (req: CoapRequest) => {
      try {
        req.getFormat();
      } catch(err) {
        expect(err).to.exist; // content not set in get req.
      }

      let ref = req.getConnectionRef();
      expect(ref).to.exist.and.be.a("Number");

      req.setResponseCode(205);
      let buf = new ArrayBuffer(data.length);
      let bufView = new Uint8Array(buf);
      for (let i = 0; i < data.length; i ++) {
        bufView[i] = data.charCodeAt(i);
      }
      req.setResponsePayload(0, buf);
      req.responseReady();
    });
    await dev.start();
    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
    const coapReq = conn.createCoapRequest("GET", path);
    const coapResp = await coapReq.execute();

    expect(coapResp.getResponseStatusCode()).to.equal(205);
    expect(coapResp.getResponseContentFormat()).to.equal(0);
    expect((Buffer.from(coapResp.getResponsePayload())).toString('utf8')).to.equal(data);
  });

  it('post coap request', async () => {
    let data = "Hello world";
    let path = '/hello/{planet}';
    let called = false;
    dev.addCoapEndpoint(CoapMethod.POST, path, (req: CoapRequest) => {
      called = true;
      let format = req.getFormat();
      expect(format).to.equal(0);

      let param = req.getParameter("planet");
      expect(param).to.exist.and.equal("world");

      let payload = req.getPayload();
      expect((Buffer.from(payload)).toString('utf8')).to.equal(data);

      req.setResponseCode(200);
      let buf = new ArrayBuffer(data.length);
      let bufView = new Uint8Array(buf);
      for (let i = 0; i < data.length; i ++) {
        bufView[i] = data.charCodeAt(i);
      }
      req.setResponsePayload(0, buf);
      req.responseReady();
    });
    await dev.start();
    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
    const coapReq = conn.createCoapRequest("POST", '/hello/world');
    let buf = new ArrayBuffer(data.length);
    let bufView = new Uint8Array(buf);
    for (let i = 0; i < data.length; i ++) {
      bufView[i] = data.charCodeAt(i);
    }
    coapReq.setRequestPayload(0, buf);
    const coapResp = await coapReq.execute();

    expect(coapResp.getResponseStatusCode()).to.equal(200);
    expect(coapResp.getResponseContentFormat()).to.equal(0);
    expect((Buffer.from(coapResp.getResponsePayload())).toString('utf8')).to.equal(data);
    expect(called).to.be.true;
  });

});
