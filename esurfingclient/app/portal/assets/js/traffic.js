/** 采样是否新鲜 (后端每秒一帧) */
function trafficIsFresh(data, now = Date.now()) {
    return Number.isFinite(data.timestamp) && now >= data.timestamp - 2000 && now - data.timestamp < 6000;
}

document.addEventListener('alpine:init', () => {
    // 数据来自本程序的 /api/traffic; 平台没实现采样 (或还没来得及采第一帧) 时保持原样
    Alpine.store('networkTools', { enabled: false, hotspotControls: false, timestamp: 0 });
    Alpine.data('trafficPanel', () => ({
        adapters: [], selectedId: '', fresh: false, timestamp: 0, experimental: false,
        message: '正在读取网速…', timer: null, request: null, disposed: false, lastProbe: 0,
        init() {
            this.selectedId = storage.get('esurfing-traffic-adapter');
            this.refresh();
            this.timer = startPolling(() => this.refresh(), 1000, 'dashboard');
        },
        destroy() {
            this.disposed = true;
            clearInterval(this.timer);
            if (this.request) this.request.abort();
        },
        selectAdapter() {
            storage.set('esurfing-traffic-adapter', this.selectedId);
        },
        get adapter() { return this.adapters.find(item => item.id === this.selectedId); },
        get downloadText() { return formatBytes(this.live?.(this.adapter?.down), true); },
        get uploadText() { return formatBytes(this.live?.(this.adapter?.up), true); },
        /** 只有采样新鲜且网卡已连接时才显示速度 */
        live(value) { return this.fresh && this.adapter?.connected !== false ? value : null; },
        async refresh() {
            if (this.request || this.disposed) return;
            if (!Alpine.store('networkTools').enabled && Date.now() - this.lastProbe < 5000) return;
            this.lastProbe = Date.now();
            if (!trafficIsFresh({ timestamp: this.timestamp })) {
                this.fresh = false;
                Alpine.store('networkTools').timestamp = 0;
            }
            this.request = new AbortController();
            try {
                const { ok, data } = await httpJson('/api/traffic', { timeout: 4000, signal: this.request.signal });
                if (this.disposed) return;
                if (!ok) throw new Error('网速采集不可用（当前平台暂不支持或程序未运行）。');
                if (!Array.isArray(data.adapters)) throw new Error('网速数据格式无效。');
                Alpine.store('networkTools').enabled = true;
                Alpine.store('networkTools').hotspotControls = data.hotspotControls === true;
                this.experimental = data.experimental === true;
                if (!trafficIsFresh(data) || !Array.isArray(data.adapters)) throw new Error('网速数据已过期，正在重试…');
                Alpine.store('networkTools').timestamp = data.timestamp;
                this.adapters = data.adapters;
                if (!this.selectedId) this.selectedId = data.defaultId || this.adapters[0]?.id || '';
                this.fresh = true;
                this.timestamp = data.timestamp;
                this.message = !this.adapters.length ? '没有找到可用的网卡，请检查校园网连接。' :
                    !this.adapters.some(item => item.connected !== false) ? '所有网卡都未连接，接上后自动开始统计。' :
                    !this.adapter ? '所选网卡已不在列表里，请重新选择。' :
                    this.adapter.connected === false ? '所选网卡未连接，接上后自动开始统计。' :
                    this.adapter.down === null ? '正在采样，稍后显示速度。' : '每秒更新 · 当前网卡全部流量';
            } catch (error) {
                this.fresh = false;
                Alpine.store('networkTools').timestamp = 0;
                this.message = error.status === 0 ? '读取超时，正在重试…' : error.message;
            } finally {
                this.request = null;
            }
        }
    }));
});
