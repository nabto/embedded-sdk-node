import { NabtoDevice, DeviceConfiguration, DeviceOptions, LogMessage, ConnectionEvent, ConnectionEventCallback, DeviceEventCallback, DeviceEvent, ConnectionRef, Connection } from "../NabtoDevice";

var nabto_device = require('bindings')('nabto_device');

export class ConnectionImpl implements Connection {
    nabtoDevice: any;

    constructor(dev: any) {
        this.nabtoDevice = dev;
    }

    getClientFingerprint(connectionRef: ConnectionRef): string
    {
        return this.nabtoDevice.connectionGetClientFingerprint(connectionRef);
    }

    isLocal(connectionRef: any): Boolean {
        return this.nabtoDevice.connectionIsLocal(connectionRef);
    }

    isPasswordAuthenticated(connectionRef: ConnectionRef): Boolean
    {
        return this.nabtoDevice.connectionIsPasswordAuthenticated(connectionRef);
    }

    getPasswordAuthenticationUsername(connectionRef: ConnectionRef): string
    {
        return this.nabtoDevice.connectionGetPasswordAuthUsername(connectionRef);
    }

}


export class NabtoDeviceImpl implements NabtoDevice {
    nabtoDevice: any;

    connectionEventListeners: ConnectionEventCallback[] = [];
    deviceEventListeners: DeviceEventCallback[] = [];

    constructor() {
        this.nabtoDevice = new nabto_device.NabtoDevice();
        this.connection = new ConnectionImpl(this.nabtoDevice)
    }

    stop() {
        this.nabtoDevice.stop();
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

    onConnectionEvent(fn: ConnectionEventCallback)
    {
        this.connectionEventListeners.push(fn);
        if (this.connectionEventListeners.length == 1) {
            this.startConnectionEventListener();
        }
    }

    onDeviceEvent(fn: DeviceEventCallback): void
    {
        this.deviceEventListeners.push(fn);
        if (this.deviceEventListeners.length == 1) {
            this.startDeviceEventListener();
        }

    }

    connection: Connection;


    private async startDeviceEventListener(): Promise<void>
    {
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

    private async startConnectionEventListener(): Promise<void>
    {
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
