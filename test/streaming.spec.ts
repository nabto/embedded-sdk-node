import 'mocha'
import { expect } from 'chai'
import { env } from 'process';
import { Connection, NabtoClient, NabtoClientFactory } from 'edge-client-node'
import { Stream, DeviceOptions, LogMessage, NabtoDevice, NabtoDeviceFactory } from '../src/NabtoDevice/NabtoDevice';

const logLevel = env.NABTO_LOG_LEVEL;

function createClientWithConn(): [cli: NabtoClient, conn: Connection] {
  let cli = NabtoClientFactory.create();
  if (logLevel) {
    cli.setLogLevel(logLevel);
    cli.setLogCallback((logMessage) => {
      let now = new Date();
      if (logMessage.severity.length < 5) {
        logMessage.severity += "_"
      }
      console.log(`[cli] ${now.toISOString()} [${logMessage.severity}]: ${logMessage.message}`);
    });
  }
  let key = cli.createPrivateKey();
  let conn = cli.createConnection();
  conn.setOptions({ProductId: "pr-foobar", DeviceId: "de-foobar", Local: true, Remote: false, PrivateKey: key});
  return [cli, conn];
}

function bufferFromString(data: string) {
  let buf = new ArrayBuffer(data.length);
  let bufView = new Uint8Array(buf);
  for (let i = 0; i  < data.length; i++) {
    bufView[i] = data.charCodeAt(i);
  }
  return buf;
}

function stringFromBuffer(data: ArrayBuffer) {
  return (Buffer.from(data)).toString('utf8');
}

describe.only('streaming', () => {
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
        console.log(`[dev] ${now.toISOString()} [${logMessage.severity}]: ${logMessage.message}`);
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

  it('open stream', async () => {
    let deviceStream: Stream | undefined;
    dev.addStream(4242, async (stream) => {
      deviceStream = stream;
      await stream.accept();
    });
    await dev.start();

    [cli, conn] = createClientWithConn();
    await conn.connect();

    let stream = conn.createStream();
    await stream.open(4242).catch((err) => {expect(err).to.be.undefined});
    await stream.close().catch((err) => {expect(err).to.be.undefined});
    stream.abort();
  });

  it('open stream readSome', async () => {
    let deviceStream: Stream | undefined;
    let testData = "hello World";
    let buf = bufferFromString(testData);

    let resolver: (value: void | PromiseLike<void>) => void;
    let rejecter: (value: any | PromiseLike<any>) => void;
    const p = new Promise<void>((resolve, reject) => {
      resolver = resolve;
      rejecter = reject;
    })

    dev.addStream(4242, async (stream) => {
      deviceStream = stream;
      try {
        await stream.accept();
        let readBuf = await stream.readSome();
        console.log("readBuf: ", readBuf);
        console.log("buf: ", buf);
        expect(readBuf.byteLength).to.equal(buf.byteLength);
        let readBufStr = stringFromBuffer(readBuf);
        expect(readBufStr).to.equal(testData);
        resolver();
      } catch (err) {
        rejecter(err);
      }
    });
    await dev.start();

    [cli, conn] = createClientWithConn();
    await conn.connect();

    let stream = conn.createStream();
    await stream.open(4242).catch((err) => {expect(err).to.be.undefined});

    await stream.write(buf);

    await p;

    await stream.close().catch((err) => {expect(err).to.be.undefined});
    stream.abort();
  });

});
