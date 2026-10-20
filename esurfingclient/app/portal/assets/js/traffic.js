function trafficRateText(value) {
    if (typeof value !== 'number' || !Number.isFinite(value) || value < 0) return '—';
    if (value >= 1048576) return (value / 1048576).toFixed(2) + ' MiB/s';
    if (value >= 1024) return (value / 1024).toFixed(2) + ' KiB/s';
    return Math.round(value) + ' B/s';
}

function trafficIsFresh(data, now = Date.now()) {
    return Number.isFinite(data.timestamp) && now >= data.timestamp - 2000 && now - data.timestamp < 6000;
}

document.addEventListener('alpine:init', () => {
    // 数据来自本程序的 /api/traffic; 平台没实现采样 (或还没来得及采第一帧) 时保持原样
    Alpine.store('networkTools', { enabled: false, hotspotControls: false, timestamp: 0 });
    Alpine.data('trafficPanel', () => ({
        adapters: [], selectedId: '', fresh: false, timestamp: 0,
        message: '正在读取网速…', timer: null, request: null, disposed: false, lastProbe: 0,
        init() {
            try { this.selectedId = localStorage.getItem('esurfing-traffic-adapter') || ''; } catch (_) {}
            this.refresh();
            this.timer = setInterval(() => {
                if (document.hidden || Alpine.store('main').activePanel !== 'dashboard') return;
                this.refresh();
            }, 1000);
        },
        destroy() {
            this.disposed = true;
            clearInterval(this.timer);
            if (this.request) this.request.abort();
        },
        selectAdapter() {
            try { localStorage.setItem('esurfing-traffic-adapter', this.selectedId); } catch (_) {}
        },
        get adapter() { return this.adapters.find(item => item.id === this.selectedId); },
        get downloadText() { return trafficRateText(this.fresh && this.adapter?.connected !== false ? this.adapter?.down : null); },
        get uploadText() { return trafficRateText(this.fresh && this.adapter?.connected !== false ? this.adapter?.up : null); },
        async refresh() {
            if (this.request || this.disposed) return;
            if (!Alpine.store('networkTools').enabled && Date.now() - this.lastProbe < 5000) return;
            this.lastProbe = Date.now();
            if (!trafficIsFresh({ timestamp: this.timestamp })) {
                this.fresh = false;
                Alpine.store('networkTools').timestamp = 0;
            }
            this.request = new AbortController();
            const timeout = setTimeout(() => this.request?.abort(), 4000);
            try {
                const response = await fetch('/api/traffic', { cache: 'no-store', signal: this.request.signal });
                if (!response.ok) throw new Error('网速采集不可用（当前平台暂不支持或程序未运行）。');
                const data = await response.json();
                if (this.disposed) return;
                if (!Array.isArray(data.adapters)) throw new Error('网速数据格式无效。');
                Alpine.store('networkTools').enabled = true;
                Alpine.store('networkTools').hotspotControls = data.hotspotControls === true;
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
                this.message = error.name === 'AbortError' ? '读取超时，正在重试…' : error.message;
            } finally {
                clearTimeout(timeout);
                this.request = null;
            }
        }
    }));
});
