const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
let createPanel;
const tools = { hotspotControls: true, hotspotBridge: null, timestamp: Date.now() };
let loadHotspot = false;
const context = {
    URL, location: { origin: 'http://localhost:9999' },
    document: { addEventListener(_, callback) { if (loadHotspot) callback(); } },
    Alpine: { data(_, factory) { createPanel = factory; }, store() { return tools; } }
};
vm.createContext(context);
vm.runInContext(fs.readFileSync(path.join(__dirname, '../../esurfingclient/app/portal/assets/js/traffic.js'), 'utf8'), context);
loadHotspot = true;
vm.runInContext(fs.readFileSync(path.join(__dirname, '../../esurfingclient/app/portal/assets/js/hotspot.js'), 'utf8'), context);
assert.equal(context.hotspotBridgeEndpoint('http://127.0.0.1:19999'), 'http://127.0.0.1:19999');
for (const url of ['https://127.0.0.1:19999', 'http://evil.test', 'http://127.0.0.1:19999/other',
    'http://user:password@127.0.0.1:19999', 'http://127.0.0.1:19999?secret=x', 'file:///tmp', 'invalid']) {
    assert.equal(context.hotspotBridgeEndpoint(url), null);
}
(async () => {
    const panel = createPanel();
    let requests = 0;
    panel.call = async () => { requests++; return { adapters: [], token: 'test', busy: false, message: '' }; };
    tools.hotspotBridge = { ready: false, code: 'AccessDenied', message: 'Permission denied' };
    await panel.refresh();
    assert.equal(requests, 0);
    assert.equal(panel.message, 'Permission denied');
    tools.hotspotBridge = { ready: true, url: 'http://127.0.0.1:19999', origins: ['http://localhost:8888'] };
    await panel.refresh();
    assert.equal(requests, 0);
    tools.hotspotBridge.origins = ['http://localhost:9999'];
    await panel.refresh();
    assert.equal(requests, 1);
    assert.equal(panel.online, true);
    panel.adapters = [{ id: 'test', state: 'Off', capability: 'Enabled', connected: true }];
    panel.selectedId = 'test';
    assert.equal(panel.canStart, true);
    tools.timestamp = Date.now() - 10000;
    assert.equal(panel.canStart, false, 'Expired telemetry must immediately block starting hotspot.');
    panel.adapters[0].state = 'On';
    assert.equal(panel.canStop, false, 'Expired telemetry must block sending a stale stop command.');
    await panel.change('stop');
    await panel.refresh();
    assert.equal(requests, 1, 'Stale helper data must not send bridge requests.');
    assert.equal(panel.online, false);
    console.log('PASS: configurable loopback endpoint, unsafe URLs, origin mismatch and disabled bridge');
})().catch(error => { console.error(error); process.exitCode = 1; });
