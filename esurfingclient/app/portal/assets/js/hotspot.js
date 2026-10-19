// Telemetry may configure the port, but must never redirect control requests off this computer.
function hotspotBridgeEndpoint(value) {
    try {
        const url = new URL(value);
        return url.protocol === 'http:' && url.hostname === '127.0.0.1' && !url.username && !url.password &&
            url.pathname === '/' && !url.search && !url.hash ? url.origin : null;
    } catch (_) { return null; }
}

document.addEventListener('alpine:init', () => {
    Alpine.data('hotspotPanel', () => ({
        adapters: [], selectedId: '', token: '', online: false, busy: false,
        sending: false, polling: false, message: '', error: '', timer: null, disposed: false,
        init() {
            this.refresh();
            this.timer = setInterval(() => {
                if (!document.hidden && Alpine.store('main').activePanel === 'dashboard') this.refresh();
            }, 2000);
        },
        destroy() { this.disposed = true; clearInterval(this.timer); },
        get adapter() { return this.adapters.find(row => row.id === this.selectedId); },
        get helperFresh() { return trafficIsFresh({ timestamp: Alpine.store('networkTools').timestamp }); },
        get locked() { return this.sending || this.busy || this.adapters.some(row => ['On', 'InTransition'].includes(row.state)); },
        get canStart() { return this.online && this.helperFresh && !this.locked && this.adapter?.state === 'Off' && this.adapter?.capability === 'Enabled' && this.adapter?.connected; },
        get canStop() { return this.online && this.helperFresh && !this.sending && !this.busy && this.adapter?.state === 'On'; },
        get stateText() {
            if (!this.online) return '未连接';
            if (this.busy || this.sending) return '正在切换';
            return ({ On: '已开启', Off: '已关闭', InTransition: '正在切换', Unknown: '未知' })[this.adapter?.state] || '不可用';
        },
        async call(path, method = 'GET') {
            if (method === 'POST' && !this.helperFresh) throw new Error('采集数据已过期，请重新启动辅助程序。');
            const base = hotspotBridgeEndpoint(Alpine.store('networkTools').hotspotBridge?.url);
            if (!base) throw new Error('热点控制地址无效，请重新启动辅助程序。');
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 5000);
            try {
                const response = await fetch(base + path, {
                    method, cache: 'no-store', signal: controller.signal,
                    headers: method === 'POST' ? { 'X-ESurfing-Token': this.token } : {}
                });
                const data = await response.json();
                if (!response.ok) throw new Error(data.message || '热点控制请求失败');
                return data;
            } finally { clearTimeout(timeout); }
        },
        async refresh() {
            if (this.polling || this.disposed) return;
            if (!Alpine.store('networkTools')?.hotspotControls) return;
            if (!this.helperFresh) {
                this.online = false;
                this.token = '';
                this.message = '采集数据已过期，热点控制已暂停，请重新启动辅助程序。';
                return;
            }
            const bridge = Alpine.store('networkTools').hotspotBridge;
            if (!bridge?.ready || !hotspotBridgeEndpoint(bridge.url) || !bridge.origins?.includes(location.origin)) {
                this.online = false;
                this.message = bridge?.message || (!bridge ? '请更新 Windows 辅助程序以启用热点控制。' : '页面地址与辅助程序配置不一致，请核对 WebPort。');
                return;
            }
            this.polling = true;
            try {
                const data = await this.call('/status');
                if (this.disposed) return;
                this.adapters = data.adapters;
                if (!this.adapters.some(row => row.id === this.selectedId)) {
                    this.selectedId = this.adapters.find(row => row.state === 'On')?.id || data.defaultId || this.adapters[0]?.id || '';
                }
                this.token = data.token;
                this.busy = data.busy;
                this.message = data.message || (this.adapter?.capability !== 'Enabled' ? '此网卡暂不支持热点，请打开系统热点设置检查。' : '按需手动开启，关闭网页不会关闭热点。');
                this.online = true;
            } catch (_) {
                this.online = false;
                this.message = '热点控制未启动，请双击“启动校园网与网速”。';
            } finally { this.polling = false; }
        },
        async change(action) {
            if ((action === 'start' && !this.canStart) || (action === 'stop' && !this.canStop)) return;
            if (!['start', 'stop'].includes(action)) return;
            this.sending = true;
            this.error = '';
            try {
                const data = await this.call('/' + action + '?adapter=' + encodeURIComponent(this.selectedId), 'POST');
                this.message = data.message;
                await this.refresh();
            } catch (error) {
                this.error = error.name === 'AbortError' || error instanceof TypeError ?
                    '未收到操作结果，请查看热点实际状态后再操作。' : error.message;
                await this.refresh();
            } finally { this.sending = false; }
        }
    }));
});
