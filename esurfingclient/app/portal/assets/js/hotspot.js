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
        get locked() { return this.sending || this.busy || this.adapters.some(row => ['On', 'InTransition'].includes(row.state)); },
        get canStart() { return this.online && !this.locked && this.adapter?.state === 'Off' && this.adapter?.capability === 'Enabled' && this.adapter?.connected; },
        get canStop() { return this.online && !this.sending && !this.busy && this.adapter?.state === 'On'; },
        get stateText() {
            if (!this.online) return '未连接';
            if (this.busy || this.sending) return '正在切换';
            return ({ On: '已开启', Off: '已关闭', InTransition: '正在切换', Unknown: '未知' })[this.adapter?.state] || '不可用';
        },
        async call(path, method = 'GET') {
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 5000);
            try {
                const response = await fetch('http://127.0.0.1:18889' + path, {
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
