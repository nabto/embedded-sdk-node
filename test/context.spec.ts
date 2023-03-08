import 'mocha'
import { strict as assert } from 'node:assert';
import chai from 'chai';

import { NabtoDevice } from '../src/NabtoDevice/NabtoDevice'

const expect = chai.expect;

describe('lifecycle', () => {
  it('test get version', () => {
    let dev = new NabtoDevice();
    let version = dev.version();
    expect(version).to.exist;
    expect(version).to.be.a("string");
  });


});
