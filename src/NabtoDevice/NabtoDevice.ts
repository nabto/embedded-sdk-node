var nabto_device = require('bindings')('nabto_device');


export class NabtoDevice {
    nabtoDevice: any;

    constructor() {
        this.nabtoDevice = new nabto_device.NabtoDevice();
    }

    version(): string {
        return this.nabtoDevice.getVersion();
    }
}
