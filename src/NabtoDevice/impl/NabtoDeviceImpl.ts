import { NabtoDevice, DeviceConfiguration, DeviceOptions, LogMessage, ConnectionEvent, ConnectionEventCallback, DeviceEventCallback, DeviceEvent, ConnectionRef, Connection, CoapMethod, CoapRequestCallback, CoapRequest, AuthorizationRequestCallback, AuthorizationRequest, Experimental, IceServersRequest, IceServer, StreamCallback, Stream } from "../NabtoDevice";

var nabto_device = require('bindings')('nabto_device');

export class IceServersRequestImpl implements IceServersRequest {
  iceRequest: any;

  execute(identifier: string): Promise<IceServer[]> {
    return this.iceRequest.send(identifier).then(() => {
      return this.iceRequest.getResponse();
    });
  }

  constructor(device: any) {
    this.iceRequest = new nabto_device.IceServersRequest(device);
  }
}

export class ExperimentalImpl implements Experimental {
  nabtoDevice: any;

  constructor(dev: any) {
    this.nabtoDevice = dev;
  }

  setRawPrivateKey(key: string): void {
    this.nabtoDevice.setRawPrivateKey(key);
  }

  createIceServersRequest(): IceServersRequest {
    return new IceServersRequestImpl(this.nabtoDevice);
  }
}

export class ConnectionImpl implements Connection {
  nabtoDevice: any;

  constructor(dev: any) {
    this.nabtoDevice = dev;
  }

  getClientFingerprint(connectionRef: ConnectionRef): string {
    return this.nabtoDevice.connectionGetClientFingerprint(connectionRef);
  }

  isLocal(connectionRef: any): Boolean {
    return this.nabtoDevice.connectionIsLocal(connectionRef);
  }

  isPasswordAuthenticated(connectionRef: ConnectionRef): Boolean {
    return this.nabtoDevice.connectionIsPasswordAuthenticated(connectionRef);
  }

  getPasswordAuthenticationUsername(connectionRef: ConnectionRef): string {
    return this.nabtoDevice.connectionGetPasswordAuthUsername(connectionRef);
  }

}


export class NabtoDeviceImpl implements NabtoDevice {
  nabtoDevice: any;

  connectionEventListeners: ConnectionEventCallback[] = [];
  deviceEventListeners: DeviceEventCallback[] = [];
  coapEndpoints: CoapEndpointHandler[] = [];
  streamListeners: StreamListener[] = [];
  authHandler: AuthRequestHandler | undefined;

  constructor() {
    this.nabtoDevice = new nabto_device.NabtoDevice();
    this.connection = new ConnectionImpl(this.nabtoDevice);
    this.experimental = new ExperimentalImpl(this.nabtoDevice);
  }

  stop() {
    this.nabtoDevice.stop();
    for (let e of this.coapEndpoints) {
      e.stop();
    }
    this.coapEndpoints = [];
    for (let s of this.streamListeners) {
      s.stop();
    }
    this.streamListeners = [];
    if (this.authHandler) {
      this.authHandler.stop();
    }
    this.authHandler = undefined;
  }

  start(): Promise<void> {
    return this.nabtoDevice.start();
  }

  version(): string {
    return this.nabtoDevice.getVersion();
  }

  setOptions(opts: DeviceOptions): void {
    return this.nabtoDevice.setOptions(opts);
  }

  createPrivateKey(): string {
    return this.nabtoDevice.createPrivateKey();
  }

  setLogLevel(logLevel: string) {
    this.nabtoDevice.setLogLevel(logLevel);
  }

  setLogCallback(callback: (logMessage: LogMessage) => void) {
    this.nabtoDevice.setLogCallback(callback);
  }

  getConfiguration(): DeviceConfiguration {
    return this.nabtoDevice.getConfiguration();
  }

  setBasestationAttach(enable: Boolean) {
    return this.nabtoDevice.setBasestationAttach(enable);
  }

  onConnectionEvent(fn: ConnectionEventCallback) {
    this.connectionEventListeners.push(fn);
    if (this.connectionEventListeners.length == 1) {
      this.startConnectionEventListener();
    }
  }

  onDeviceEvent(fn: DeviceEventCallback): void {
    this.deviceEventListeners.push(fn);
    if (this.deviceEventListeners.length == 1) {
      this.startDeviceEventListener();
    }

  }

  onAuthorizationRequest(fn: AuthorizationRequestCallback): void {
    if (this.authHandler == undefined) {
      // Not currently listening
      this.authHandler = new AuthRequestHandler(this.nabtoDevice, fn);
    } else {
      throw new Error("Multiple Authorization request listeners are not allowed");
    }
  }

  mdnsAddSubtype(type: string): void {
    return this.nabtoDevice.mdnsAddSubtype(type);
  }

  mdnsAddTxtItem(key: string, value: string): void {
    return this.nabtoDevice.mdnsAddTxtItem(key, value);
  }

  createServerConnectToken(): string {
    return this.nabtoDevice.createServerConnectToken();
  }

  addServerConnectToken(sct: string): void {
    return this.nabtoDevice.addServerConnectToken(sct);
  }

  areServerConnectTokensSync(): Boolean {
    return this.nabtoDevice.areServerConnectTokensSync();
  }

  addCoapEndpoint(method: CoapMethod, path: string, cb: CoapRequestCallback): void {
    this.coapEndpoints.push(new CoapEndpointHandler(this.nabtoDevice, method, path, cb));
  }

  addStream(port: number, cb: StreamCallback): number {
    let s = new StreamListener(this.nabtoDevice, port, cb);
    this.streamListeners.push(s);
    return s.getStreamPort();
  }

  addTcpTunnelService(serviceId: string, serviceType: string, host: string, port: number): void {
    return this.nabtoDevice.addTcpTunnelService(serviceId, serviceType, host, port);
  }

  removeTcpTunnelService(serviceId: string): void {
    return this.nabtoDevice.removeTcpTunnelService(serviceId);
  }

  connection: Connection;
  experimental: Experimental;


  private async startDeviceEventListener(): Promise<void> {
    try {
      await this.nabtoDevice.notifyDeviceEvent();
      let ev: DeviceEvent = this.nabtoDevice.getCurrentDeviceEvent();
      for (let f of this.deviceEventListeners) {
        f(ev);
      }
      return this.startDeviceEventListener();
    } catch (err) {
      // TODO: handle... probably just closing down
    }
  }

  private async startConnectionEventListener(): Promise<void> {
    try {
      await this.nabtoDevice.notifyConnectionEvent();
      let ev: ConnectionEvent = this.nabtoDevice.getCurrentConnectionEvent();
      let ref: ConnectionRef = this.nabtoDevice.getCurrentConnectionRef();
      for (let f of this.connectionEventListeners) {
        f(ev, ref);
      }
      return this.startConnectionEventListener();
    } catch (err) {
      // TODO: handle... probably just closing down
    }
  }
}

export class CoapEndpointHandler {
  nabtoDevice: any;
  ep: any;

  cb: CoapRequestCallback;

  constructor(device: any, method: CoapMethod, path: string, cb: CoapRequestCallback) {
    this.nabtoDevice = device;
    this.cb = cb;
    this.ep = new nabto_device.CoapEndpoint(device, method, path);
    this.nextReq();
  }

  stop(): void {
    this.ep.stop();
  }

  async nextReq(): Promise<void> {
    try {
      await this.ep.notifyRequest();
      let nativeReq = this.ep.getCurrentRequest();
      let req = new CoapRequestImpl(nativeReq);
      this.cb(req);
      this.nextReq();
    } catch (err) {
      // TODO: handle... probably just closing down
    }

  }
}

export class CoapRequestImpl implements CoapRequest {
  req: any;

  constructor(nativeReq: any) {
    this.req = new nabto_device.CoapRequest(nativeReq);
  }

  getFormat(): Number {
    return this.req.getFormat();
  }

  getPayload(): ArrayBuffer {
    return this.req.getPayload();
  }

  getConnectionRef(): ConnectionRef {
    return this.req.getConnectionRef();
  }

  getParameter(parameterName: string): string {
    return this.req.getParameter(parameterName);
  }

  sendErrorResponse(code: Number, message: string): void {
    return this.req.sendErrorResponse(code, message);
  }

  setResponseCode(code: Number): void {
    return this.req.setResponseCode(code);
  }

  setResponsePayload(format: Number, payload: ArrayBuffer): void {
    return this.req.setResponsePayload(format, payload);
  }

  responseReady(): void {
    return this.req.responseReady();
  }

}

export class StreamListener {
  nabtoDevice: any;
  listener: any;

  cb: StreamCallback;

  constructor(device: any, port: number, cb: StreamCallback) {
    this.nabtoDevice = device;
    this.cb = cb;
    this.listener = new nabto_device.StreamListener(device, port);
    this.nextStream();
  }

  stop(): void {
    this.listener.stop();
  }

  getStreamPort(): number {
    return this.listener.getStreamPort();
  }

  async nextStream(): Promise<void> {
    try {
      await this.listener.notifyStream();
      let nativeStream = this.listener.getCurrentStream();
      let stream = new StreamImpl(this.nabtoDevice, nativeStream);
      this.cb(stream);
      this.nextStream();
    } catch (err) {
      // TODO: handle... probably just closing down
    }
  }
}

export class StreamImpl implements Stream {
  stream: any;

  constructor(device: any, nativeStream: any) {
    this.stream = new nabto_device.Stream(device, nativeStream);
  }

  accept(): Promise<void> {
    return this.stream.accept();
  }

  getConnectionRef(): ConnectionRef {
    return this.stream.getConnectionRef();
  }

  readSome(): Promise<ArrayBuffer> {
    return this.stream.readSome().then(() => {
      return this.stream.getData();
    });
  }

  readAll(length: number): Promise<ArrayBuffer> {
    return this.stream.readAll(length).then(() => {
      return this.stream.getData();
    });
  }

  write(data: ArrayBuffer): Promise<void> {
    return this.stream.write(data);
  }

  close(): Promise<void> {
    return this.stream.close()
  }

  abort(): void {
    return this.stream.abort();
  }
}

export class AuthRequestHandler {
  nabtoDevice: any;
  auth: any;

  cb: AuthorizationRequestCallback;

  constructor(device: any, cb: AuthorizationRequestCallback) {
    this.nabtoDevice = device;
    this.cb = cb;
    this.auth = new nabto_device.AuthHandler(device);
    this.nextReq();
  }

  stop(): void {
    this.auth.stop();
  }

  async nextReq(): Promise<void> {
    try {
      await this.auth.notifyRequest();
      let nativeReq = this.auth.getCurrentRequest();
      let req = new AuthorizationRequestImpl(nativeReq);
      this.cb(req);
      this.nextReq();
    } catch (err) {
      // TODO: handle... probably just closing down
    }

  }
}

export class AuthorizationRequestImpl implements AuthorizationRequest {
  req: any;

  constructor(nativeReq: any) {
    this.req = new nabto_device.AuthRequest(nativeReq);
  }

  verdict(allowed: Boolean): void {
    return this.req.verdict(allowed);
  }

  getAction(): string {
    return this.req.getAction();
  }

  getConnectionRef(): ConnectionRef {
    return this.req.getConnectionRef();
  }

  getAttributes(): { [key: string]: string } {
    return this.req.getAttributes();
  }
}
