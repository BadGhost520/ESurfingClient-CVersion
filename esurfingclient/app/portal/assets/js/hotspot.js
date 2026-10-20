// 热点控制由本程序自己提供 (/api/hotspot/*, 同源请求):
// 不再需要回环桥接进程, 也就不需要令牌与来源校验
document.addEventListener('alpine:init', () => {
    Alpine.data('hotspotPanel', () => ({
        adapters: [], selectedId: '', online: false, busy: false, experimental: false,
        sending: false, polling: false, message: '', error: '', timer: null, disposed: false,
        init() {
            this.refresh();
            // 每次刷新都要让主程序跑一次 PowerShell 拿状态, 没必要太勤
            this.timer = setInterval(() => {
                if (!document.hidden && Alpine.store('main').activePanel === 'dashboard') this.refresh();
            }, 5000);
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
            const controller = new AbortController();
            // 开关要等 Windows 真的切完, 与后端的动作超时留一样的余量
            const timeout = setTimeout(() => controller.abort(), method === 'POST' ? 25000 : 5000);
            try {
                const response = await fetch('/api/hotspot' + path, { method, cache: 'no-store', signal: controller.signal });
                const text = await response.text();
                let data = {};
                try { data = text ? JSON.parse(text) : {}; } catch (_) { data = {}; }
                if (!response.ok) throw new Error(data.message || '热点请求失败');
                return data;
            } finally { clearTimeout(timeout); }
        },
        async refresh() {
            if (this.polling || this.disposed) return;
            if (!Alpine.store('networkTools')?.hotspotControls) return;
            if (!this.helperFresh) {
                this.online = false;
                this.message = '采集数据已过期，热点控制已暂停。';
                return;
            }
            this.polling = true;
            try {
                const data = await this.call('/status');
                if (this.disposed) return;
                this.adapters = Array.isArray(data.adapters) ? data.adapters : [];
                if (!this.adapters.some(row => row.id === this.selectedId)) {
                    this.selectedId = this.adapters.find(row => row.state === 'On')?.id || data.defaultId || this.adapters[0]?.id || '';
                }
                this.busy = data.busy === true;
                this.experimental = data.experimental === true;
                this.message = data.message || (this.adapter?.capability !== 'Enabled' ? '此网卡暂不支持热点，请打开系统热点设置检查。' : '按需手动开启，关闭网页不会关闭热点。');
                this.online = true;
            } catch (_) {
                this.online = false;
                this.message = '热点控制不可用（当前平台或系统环境不支持）。';
            } finally { this.polling = false; }
        },
        async change(action) {
            if ((action === 'start' && !this.canStart) || (action === 'stop' && !this.canStop)) return;
            if (!['start', 'stop'].includes(action)) return;
            this.sending = true;
            this.error = '';
            try {
                const data = await this.call('/' + action + '?adapter=' + encodeURIComponent(this.selectedId), 'POST');
                this.message = data.message || this.message;
                await this.refresh();
            } catch (error) {
                this.error = error.name === 'AbortError' || error instanceof TypeError ?
                    '未收到操作结果，请查看热点实际状态后再操作。' : error.message;
                await this.refresh();
            } finally { this.sending = false; }
        }
    }));
});
