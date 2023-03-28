import 'mocha'
import chai from 'chai';
import { env } from 'process';
import { Connection, NabtoClient, NabtoClientFactory } from 'edge-client-node'
import { AuthorizationRequest, DeviceOptions, LogMessage, NabtoDevice, NabtoDeviceFactory } from '../src/NabtoDevice/NabtoDevice';
import { decode } from 'cbor-x';
import express, {Request, Response, NextFunction} from 'express';
import { randomInt } from 'crypto';

// Workaround for https://github.com/DefinitelyTyped/DefinitelyTyped/issues/60924 to avoid using node-fetch as this would not import properly due to some esm crap
//@ts-ignore
const fetch = globalThis.fetch

const expect = chai.expect;

const logLevel = env.NABTO_LOG_LEVEL;

class TestHttpApi {
    app: any;
    port: number;
    server: any;

    constructor(port: number) {
        this.app = express();
        this.port = port;

        this.app.get('/hello', (req: Request, res: Response, next: NextFunction) => {
            res.status(200).json("Hello good sir!");
        });

        this.server = this.app.listen(port, () => {
//            console.log("express listening on port: ", port);
        });
    }

    stop() {
        this.server.close();
    }

}

describe('tunnels', () => {
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

  it('list services', async () => {
    let called = false;
    dev.onAuthorizationRequest((req: AuthorizationRequest) => {
        let action = req.getAction();
        expect(action).to.equal("TcpTunnel:ListServices");
        let ref = req.getConnectionRef();
        expect(ref).to.exist.and.be.a("Number");
        let att = req.getAttributes();
        let keys = Object.keys(att);
        expect(keys).to.be.an('Array').and.have.length(0);

        req.verdict(true);
        called = true;
    });
    await dev.start();
    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
    const coapReq = conn.createCoapRequest("GET", '/tcp-tunnels/services');
    const coapResp = await coapReq.execute();

    expect(coapResp.getResponseStatusCode()).to.equal(205);
    expect(coapResp.getResponseContentFormat()).to.equal(60);
    expect(called).to.be.true;
  });

  it('add/remove services', async () => {
    let called = 0;
    dev.onAuthorizationRequest((req: AuthorizationRequest) => {
        let action = req.getAction();
        expect(action).to.equal("TcpTunnel:ListServices");
        let ref = req.getConnectionRef();
        expect(ref).to.exist.and.be.a("Number");
        let att = req.getAttributes();
        let keys = Object.keys(att);
        expect(keys).to.be.an('Array').and.have.length(0);

        req.verdict(true);
        called++;
    });
    await dev.start();
    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();

    { // REQ 1
        const coapReq = conn.createCoapRequest("GET", '/tcp-tunnels/services');
        const coapResp = await coapReq.execute();
        expect(coapResp.getResponseStatusCode()).to.equal(205);
        expect(coapResp.getResponseContentFormat()).to.equal(60);
        let payload = coapResp.getResponsePayload();
        let services = decode(Buffer.from(payload));
        expect(services).to.be.an('Array').and.have.length(0);
    }

    dev.addTcpTunnelService("foo", "bar", "127.0.0.1", 8080);
    { // REQ 2
        const coapReq = conn.createCoapRequest("GET", '/tcp-tunnels/services');
        const coapResp = await coapReq.execute();
        expect(coapResp.getResponseStatusCode()).to.equal(205);
        expect(coapResp.getResponseContentFormat()).to.equal(60);
        let payload = coapResp.getResponsePayload();
        let services = decode(Buffer.from(payload));
        expect(services).to.be.an('Array').and.have.length(1);
        expect(services[0]).to.equal("foo");
    }

    dev.removeTcpTunnelService("foo");
    { // REQ 3
        const coapReq = conn.createCoapRequest("GET", '/tcp-tunnels/services');
        const coapResp = await coapReq.execute();
        expect(coapResp.getResponseStatusCode()).to.equal(205);
        expect(coapResp.getResponseContentFormat()).to.equal(60);
        let payload = coapResp.getResponsePayload();
        let services = decode(Buffer.from(payload));
        expect(services).to.be.an('Array').and.have.length(0);
    }
    expect(called).to.equal(3);
  });


  it('open tunnel', async () => {
    let port = randomInt(8000, 65000);
    let cliLocalPort = randomInt(8000, 65000);
    dev.onAuthorizationRequest((req: AuthorizationRequest) => {
        req.verdict(true);
    });
    dev.addTcpTunnelService("http", "http", "127.0.0.1", port);
    let http = new TestHttpApi(port);
    await dev.start();
    cli = NabtoClientFactory.create();
    let key = cli.createPrivateKey();
    conn = cli.createConnection();
    conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
    await conn.connect();
    let tunnel = conn.createTCPTunnel();
    await tunnel.open("http", cliLocalPort);

    const response = await fetch(`http://127.0.0.1:${cliLocalPort}/hello`);
    const data = await response.json();
    expect(data).to.exist;
    expect(data).to.be.a("String");
    expect(data).to.equal("Hello good sir!");

    await tunnel.close();
    http.stop();
  });

});
