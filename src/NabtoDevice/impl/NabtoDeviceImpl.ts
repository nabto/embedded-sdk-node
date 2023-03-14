import { NabtoDevice, DeviceConfiguration, DeviceOptions, LogMessage, ConnectionEvent, ConnectionEventCallback, DeviceEventCallback, DeviceEvent, ConnectionRef, Connection, CoapMethod, CoapRequestCallback, CoapRequest } from "../NabtoDevice";

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
    coapEndpoints: CoapEndpointHandler[] = [];

    constructor() {
        this.nabtoDevice = new nabto_device.NabtoDevice();
        this.connection = new ConnectionImpl(this.nabtoDevice)
    }

    stop() {
        this.nabtoDevice.stop();
        for (let e of this.coapEndpoints) {
            e.stop();
        }
        this.coapEndpoints = [];
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

    mdnsAddSubtype(type: string): void
    {
        return this.nabtoDevice.mdnsAddSubtype(type);
    }

    mdnsAddTxtItem(key: string, value: string): void
    {
        return this.nabtoDevice.mdnsAddTxtItem(key, value);
    }

    createServerConnectToken(): string
    {
        return this.nabtoDevice.createServerConnectToken();
    }

    addServerConnectToken(sct: string): void
    {
        return this.nabtoDevice.addServerConnectToken(sct);
    }

    areServerConnectTokensSync(): Boolean
    {
        return this.nabtoDevice.areServerConnectTokensSync();
    }

    addCoapEndpoint(method: CoapMethod, path: string, cb: CoapRequestCallback): void
    {
        this.coapEndpoints.push(new CoapEndpointHandler(this.nabtoDevice, method, path, cb));
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

export class CoapEndpointHandler
{
    nabtoDevice: any;
    ep: any;

    cb: CoapRequestCallback;

    constructor(device: any, method: CoapMethod, path: string, cb: CoapRequestCallback)
    {
        this.nabtoDevice = device;
        this.cb = cb;
        this.ep = new nabto_device.CoapEndpoint(device, method, path);
        this.nextReq();
    }

    stop(): void {
        this.ep.stop();
    }

    async nextReq(): Promise<void>
    {
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

    constructor(nativeReq: any)
    {
        this.req = new nabto_device.CoapRequest(nativeReq);
    }

    getFormat(): Number
    {
        return this.req.getFormat();
    }

    getPayload(): ArrayBuffer
    {
        return this.req.getPayload();
    }

    getConnectionRef(): ConnectionRef
    {
        return this.req.getConnectionRef();
    }

    getParameter(parameterName: string): string
    {
        return this.req.getParameter(parameterName);
    }

    sendErrorResponse(code: Number, message: string): void
    {
        return this.req.sendErrorResponse(code, message);
    }

    setResponseCode(code: Number): void
    {
        return this.req.setResponseCode(code);
    }

    setResponsePayload(format: Number, payload: ArrayBuffer): void
    {
        return this.req.setResponsePayload(format, payload);
    }

    responseReady(): void
    {
        return this.req.responseReady();
    }

}
