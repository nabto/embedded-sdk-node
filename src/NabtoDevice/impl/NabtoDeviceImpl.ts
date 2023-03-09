import { NabtoDevice, DeviceConfiguration, DeviceOptions, LogMessage, ConnectionEvent, ConnectionEventCallback, DeviceEventCallback } from "../NabtoDevice";

var nabto_device = require('bindings')('nabto_device');


export class NabtoDeviceImpl implements NabtoDevice {
    nabtoDevice: any;

    connectionEventListeners: ConnectionEventCallback[] = [];
    deviceEventListeners: DeviceEventCallback[] = [];

    constructor() {
        this.nabtoDevice = new nabto_device.NabtoDevice();
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


    private startDeviceEventListener()
    {

    }

    private startConnectionEventListener()
    {

    }

    nativeDeviceEventCb()
    {

    }

}
