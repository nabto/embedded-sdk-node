import { NabtoDeviceImpl } from "./impl/NabtoDeviceImpl";


export interface DeviceOptions {
  productId: string;
  deviceId: string;
  serverUrl?: string;
  serverPort?: number;
  privateKey?: string;
  rootCerts?: string;
  appName?: string;
  appVersion?: string;
  localPort?: number;
  p2pPort?: number;
  enableMdns?: Boolean;
}

export interface LogMessage {
  message: string,
  severity: string
}

export interface DeviceConfiguration {
  productId: string;
  deviceId: string;
  appName?: string;
  appVersion?: string;
  localPort: number;
  p2pPort: number;
  deviceFingerprint: string;
}

export enum ConnectionEvent {
  OPENED = "NABTO_DEVICE_CONNECTION_EVENT_OPENED",
  CLOSED = "NABTO_DEVICE_CONNECTION_EVENT_CLOSED",
  CHANNEL_CHANGED = "NABTO_DEVICE_CONNECTION_EVENT_CHANNEL_CHANGED",
}

export enum DeviceEvent {
  ATTACHED = "NABTO_DEVICE_EVENT_ATTACHED",
  DETACHED = "NABTO_DEVICE_EVENT_DETACHED",
  CLOSED = "NABTO_DEVICE_EVENT_CLOSED",
  UNKNOWN_FINGERPRINT = "NABTO_DEVICE_EVENT_UNKNOWN_FINGERPRINT",
  WRONG_PRODUCT_ID = "NABTO_DEVICE_EVENT_WRONG_PRODUCT_ID",
  WRONG_DEVICE_ID = "NABTO_DEVICE_EVENT_WRONG_DEVICE_ID",
}

export enum CoapMethod {
  GET = "NABTO_DEVICE_COAP_GET",
  POST = "NABTO_DEVICE_COAP_POST",
  PUT = "NABTO_DEVICE_COAP_PUT",
  DELETE = "NABTO_DEVICE_COAP_DELETE",
}

export type ConnectionRef = any;

export type ConnectionEventCallback = (ev: ConnectionEvent, connectionRef: ConnectionRef) => void;

export type DeviceEventCallback = (ev: DeviceEvent) => void;

export interface CoapRequest {
  getFormat(): Number;
  getPayload(): ArrayBuffer;
  getConnectionRef(): ConnectionRef;
  getParameter(parameterName: string): string;
  sendErrorResponse(code: Number, message: string): void;
  setResponseCode(code: Number): void;
  setResponsePayload(format: Number, payload: ArrayBuffer): void;
  responseReady(): void;
}

export type CoapRequestCallback = (req: CoapRequest) => void;

export interface Stream {
  accept(): Promise<void>;
  getConnectionRef(): ConnectionRef;
  readSome(): Promise<ArrayBuffer>;
  readAll(length: number): Promise<ArrayBuffer>;
  write(data: ArrayBuffer): Promise<void>;
  close(): Promise<void>;
  abort(): void;
}

export type StreamCallback = (stream: Stream) => void;

export interface Connection {
  getClientFingerprint(connectionRef: ConnectionRef): string;
  isLocal(connectionRef: ConnectionRef): Boolean;
  isPasswordAuthenticated(connectionRef: ConnectionRef): Boolean;
  getPasswordAuthenticationUsername(connectionRef: ConnectionRef): string;
}

export interface AuthorizationRequest {
  verdict(allowed: Boolean): void;
  getAction(): string;
  getConnectionRef(): ConnectionRef;
  getAttributes(): {[key: string]: string};
}

export type AuthorizationRequestCallback = (req: AuthorizationRequest) => void;

export interface IceServer {
  username: string;
  credential: string;
  urls: string[];
}

export interface IceServersRequest {
  execute(identifier: string): Promise<IceServer[]>;
}

export interface Experimental {
  setRawPrivateKey(key: string): void;
  createIceServersRequest(): IceServersRequest;
}

export interface NabtoDevice {
  stop(): void;
  start(): Promise<void>;
  version(): string;
  setOptions(opts: DeviceOptions): void;
  createPrivateKey(): string;
  setLogLevel(logLevel: string): void;
  setLogCallback(callback: (logMessage: LogMessage) => void): void;
  getConfiguration() : DeviceConfiguration;
  setBasestationAttach(enable: Boolean): void;

  onConnectionEvent(fn: ConnectionEventCallback): void;
  onDeviceEvent(fn: DeviceEventCallback): void;
  onAuthorizationRequest(fn: AuthorizationRequestCallback): void;

  mdnsAddSubtype(type: string): void;
  mdnsAddTxtItem(key: string, value: string): void;
  createServerConnectToken(): string;
  addServerConnectToken(sct: string): void;
  areServerConnectTokensSync(): Boolean;

  addCoapEndpoint(method: CoapMethod, path: string, cb: CoapRequestCallback): void;

  // Setting the port = 0, the device uses an ephemeral port number.
  // if port = 0: this returns the chosen ephemeral port
  // if port != 0: this returns the provided port
  addStream(port: number, cb: StreamCallback): number;

  addTcpTunnelService(serviceId: string, serviceType: string, host: string, port: number): void;
  removeTcpTunnelService(serviceId: string): void;

  connection: Connection;
  experimental: Experimental;

}

export class NabtoDeviceFactory {
  static create(): NabtoDevice {
    return new NabtoDeviceImpl();
  }
}
