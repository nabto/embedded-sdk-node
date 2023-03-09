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
  OPENED = 0,
  CLOSED,
  CHANNEL_CHANGED,
}

export enum DeviceEvent {
  ATTACHED = 0,
  DETACHED,
  CLOSED,
  UNKNOWN_FINGERPRINT,
  WRONG_PRODUCT_ID,
  WRONG_DEVICE_ID,
}

export type ConnectionEventCallback = (ev: ConnectionEvent, connectionRef: any) => void;

export type DeviceEventCallback = (ev: DeviceEvent) => void;

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

}

export class NabtoDeviceFactory {
  static create(): NabtoDevice {
    return new NabtoDeviceImpl();
  }
}
