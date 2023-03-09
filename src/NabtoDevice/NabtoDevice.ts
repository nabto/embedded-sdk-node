var nabto_device = require('bindings')('nabto_device');


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
}

export interface LogMessage {
  message: string,
  severity: string
}

export class NabtoDevice {
  nabtoDevice: any;

  constructor() {
    this.nabtoDevice = new nabto_device.NabtoDevice();
  }

  stop() {
    this.nabtoDevice.stop();
  }

  start() : Promise<void> {
    return this.nabtoDevice.start();
  }

  version(): string {
    return this.nabtoDevice.getVersion();
  }

  setOptions(opts: DeviceOptions): void {
    return this.nabtoDevice.setOptions(opts);
  }

  createPrivateKey() : string {
    return this.nabtoDevice.createPrivateKey();
  }

  setLogLevel(logLevel: string) {
    this.nabtoDevice.setLogLevel(logLevel);
  }

  setLogCallback(callback: (logMessage: LogMessage) => void)
  {
      this.nabtoDevice.setLogCallback(callback);
  }


}
