import { NabtoDeviceImpl } from "./impl/NabtoDeviceImpl";


export interface DeviceOptions {
  productId: string;
  deviceId: string;
  serverUrl?: string;
  serverPort?: number;
  privateKey: string;
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

export type ConnectionRef = any;

export type ConnectionEventCallback = (ev: ConnectionEvent, connectionRef: ConnectionRef) => void;

export type DeviceEventCallback = (ev: DeviceEvent) => void;

export interface Connection {
  getClientFingerprint(connectionRef: ConnectionRef): string;
  isLocal(connectionRef: ConnectionRef): Boolean;
  isPasswordAuthenticated(connectionRef: ConnectionRef): Boolean;
  getPasswordAuthenticationUsername(connectionRef: ConnectionRef): string;
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
  mdnsAddSubtype(type: string): void;
  mdnsAddTxtItem(key: string, value: string): void;
  createServerConnectToken(): string;
  addServerConnectToken(sct: string): void;
  areServerConnectTokensSync(): Boolean;
  connection: Connection;

}

export class NabtoDeviceFactory {
  static create(): NabtoDevice {
    return new NabtoDeviceImpl();
  }
}
