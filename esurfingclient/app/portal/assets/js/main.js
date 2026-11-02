const CHANNEL_TEXT = {
    windows: 'Windows',
    linux: 'Linux',
    android: 'Android',
    ios: 'iOS',
    macos: 'MacOS'
};

const CHANNEL_VALUES = ['windows', 'linux', 'android', 'ios', 'macos'];

/** 归一化通道取值 (缺省用 Windows 通道, 与默认配置文件里的 channel: 1 一致) */
function normalizeChannel(raw) {
    if (raw === null || raw === undefined) return 'windows';

    const value = String(raw).trim().toLowerCase();
    switch (value) {
        case '1':
        case 'windows':
        case 'win':
            return 'windows';
        case '2':
        case 'linux':
            return 'linux';
        case '3':
        case 'android':
            return 'android';
        case '4':
        case 'ios':
        case 'iphone':
            return 'ios';
        case '5':
        case 'macos':
        case 'mac':
        case 'osx':
            return 'macos';
        default:
            // 后端 parse_channel_json 的默认值也是 Android
            return 'android';
    }
}

/** 归一化超时参数 (秒) */
function normalizeTimeout(raw, fallback) {
    const timeout = Math.floor(Number(raw));
    if (!Number.isFinite(timeout) || timeout <= 0) return fallback;
    return timeout;
}

const WEEK_DAYS = [
    { value: 'mon', text: '周一' },
    { value: 'tue', text: '周二' },
    { value: 'wed', text: '周三' },
    { value: 'thu', text: '周四' },
    { value: 'fri', text: '周五' },
    { value: 'sat', text: '周六' },
    { value: 'sun', text: '周日' }
];

const WEEK_DAY_VALUES = WEEK_DAYS.map(day => day.value);

const WEEK_DAY_TEXT = WEEK_DAYS.reduce((map, day) => {
    map[day.value] = day.text;
    return map;
}, {});

const TIME_RE = /^([01][0-9]|2[0-3]):[0-5][0-9]$/;

const TOAST_CLASS = {
    info: 'alert-info',
    success: 'alert-success',
    warning: 'alert-warning',
    error: 'alert-error'
};

const MAX_TIME_WINDOWS = 16;

const DEFAULT_CONN_TIMEOUT = 10;

const DEFAULT_OP_TIMEOUT = 15;

const DEFAULT_WEB_PORT = 8888;

const DEFAULT_LOG_DIR = './';

const MAX_LOG_DIR_LEN = 255;

const STATUS_INTERVAL = 5000;

const LOG_INTERVAL = 5000;

/** 默认配置 (与后端 s_default_cfg 保持一致) */
/** 配置字段表: 默认值 / 读取时归一化 / 保存时转换 (新增字段只改这里) */
/** 探测目标: 域名 + 固定 IP (与后端内置默认一致) */
const DEFAULT_PROBE_HOSTS = [
    { host: 'connectivitycheck.platform.hicloud.com', ips: ['183.61.185.93', '125.88.255.53'] },
    { host: 'connect.rom.miui.com', ips: ['202.69.4.32', '220.181.104.183'] }
];

/** IPv4 字面量校验 (和后端 probe_is_ipv4 一个要求: a.b.c.d 四段 0~255) */
function isValidIPv4(value) {
    const text = String(value == null ? '' : value).trim();
    if (!/^\d{1,3}(\.\d{1,3}){3}$/.test(text)) return false;
    return text.split('.').every(part => Number(part) <= 255);
}

/** 前端用 ipsText (逗号分隔) 编辑, 提交时再拆回 ips 数组 */
function normalizeProbeHosts(raw) {
    const list = Array.isArray(raw) ? raw : DEFAULT_PROBE_HOSTS;
    return list
        .filter(item => item && typeof item === 'object' && typeof item.host === 'string')
        .map(item => ({
            host: item.host.trim(),
            ipsText: Array.isArray(item.ips) ? item.ips.filter(isValidIPv4).join(', ') : ''
        }));
}

/** 提交给后端: 拆 ipsText, 丢掉空条目/非法 IP */
function probeHostsOut(list) {
    return (Array.isArray(list) ? list : [])
        .map(item => ({
            host: String(item.host || '').trim(),
            ips: String(item.ipsText || '').split(',').map(part => part.trim()).filter(isValidIPv4)
        }))
        .filter(item => item.host.length > 0 && item.ips.length > 0);
}

const CONFIG_FIELDS = [
    { key: 'enabled', def: false, norm: v => !!v, out: v => !!v },
    { key: 'web_external_acc', def: false, norm: v => !!v, out: v => !!v },
    { key: 'log_lv', def: 4, norm: v => clampLogLevel(v), out: v => Number(v) || 0 },
    {
        key: 'log_dir', def: DEFAULT_LOG_DIR, norm: normalizeLogDir, out: normalizeLogDir,
        check: value => (normalizeLogDir(value).length > MAX_LOG_DIR_LEN ? `日志目录过长 (最多 ${MAX_LOG_DIR_LEN} 个字符)` : null)
    },
    {
        key: 'conn_timeout', def: DEFAULT_CONN_TIMEOUT,
        norm: v => normalizeTimeout(v, DEFAULT_CONN_TIMEOUT),
        out: v => normalizeTimeout(v, DEFAULT_CONN_TIMEOUT)
    },
    {
        key: 'op_timeout', def: DEFAULT_OP_TIMEOUT,
        norm: v => normalizeTimeout(v, DEFAULT_OP_TIMEOUT),
        out: v => normalizeTimeout(v, DEFAULT_OP_TIMEOUT)
    },
    {
        key: 'web_port', def: DEFAULT_WEB_PORT,
        norm: v => (isValidWebPort(v) ? Number(v) : DEFAULT_WEB_PORT),
        out: v => Number(v),
        check: value => (isValidWebPort(value) ? null : `Web 服务端口应为 1 - 65535 的整数 (当前: ${value})`)
    },
    {
        key: 'probe_hosts', def: normalizeProbeHosts(DEFAULT_PROBE_HOSTS),
        norm: normalizeProbeHosts,
        out: probeHostsOut,
        check: value => {
            const list = probeHostsOut(value);
            if (list.length === 0) return '探测目标至少要有一条 (域名 + 合法 IPv4), 留空会回退内置默认';

            const too_many = list.find(item => item.ips.length > 4);
            if (too_many) return `${too_many.host} 配了 ${too_many.ips.length} 个 IP, 每个域名最多 4 个`;

            const total = list.reduce((sum, item) => sum + item.ips.length, 0);
            if (total > 14) return `探测目标共 ${total} 路, 最多 14 路 (认证服务器已占 2 路)`;
            return null;
        }
    }
];

function defaultConfigs() {
    const configs = {};
    CONFIG_FIELDS.forEach(field => { configs[field.key] = field.def; });
    configs.accounts = [normalizeAccount({}, true)];
    return configs;
}

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));

/** 轮询: 页面可见且停在指定面板时才执行 */
function startPolling(fn, ms, panel) {
    return setInterval(() => {
        if (document.hidden) return;
        if (panel && Alpine.store('main').activePanel !== panel) return;
        fn();
    }, ms);
}

/** 日志等级: 0~6, 非法值回默认 INFO */
function clampLogLevel(value) {
    const level = Number(value);
    return Number.isFinite(level) ? Math.min(Math.max(level, 0), 6) : 4;
}

/**
 * 账号对象归一化
 * @param {object} source 来源对象
 * @param {boolean} forSave 为真时补上后端要的 mark 字段 (写回配置用)
 */
function normalizeAccount(source = {}, forSave = false) {
    const windows = (Array.isArray(source.time_windows) ? source.time_windows : [])
        .map(window => ({
            start: String((window && window.start) || ''),
            end: String((window && window.end) || '')
        }));

    return {
        username: String(source.username || ''),
        password: String(source.password || ''),
        channel: normalizeChannel(source.channel),
        ...(forSave ? { mark: '' } : {}),
        // 显示时丢掉空行; 保存时原样带回, 避免界面上的空行被静默丢弃
        time_windows: forSave ? windows : windows.filter(window => window.start && window.end)
    };
}

/** 字节数格式化; perSecond 为真时按速率显示 */
function formatBytes(bytes, perSecond = false) {
    const size = Number(bytes);
    if (!Number.isFinite(size) || size < 0) return perSecond ? '—' : '未知大小';
    const unit = perSecond ? '/s' : '';
    if (size < 1024) return Math.round(size) + ' B' + unit;
    if (size < 1048576) return (size / 1024).toFixed(perSecond ? 2 : 1) + ' KB' + unit;
    return (size / 1048576).toFixed(2) + ' MB' + unit;
}

/** 运行时长格式化 */
function formatUptime(ms) {
    const total = Math.floor((Number(ms) || 0) / 1000);
    const days = Math.floor(total / 86400);
    const hours = Math.floor((total % 86400) / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const seconds = total % 60;
    if (days > 0) return `${days} 天 ${hours} 小时 ${minutes} 分`;
    if (hours > 0) return `${hours} 小时 ${minutes} 分 ${seconds} 秒`;
    if (minutes > 0) return `${minutes} 分 ${seconds} 秒`;
    return `${seconds} 秒`;
}

function pad2(num) {
    return String(num).padStart(2, '0');
}

/** 时间戳 (毫秒) -> HH:MM:SS */
function formatClock(value) {
    const date = value instanceof Date ? value : new Date(value);
    if (Number.isNaN(date.getTime())) return '未知';
    return `${pad2(date.getHours())}:${pad2(date.getMinutes())}:${pad2(date.getSeconds())}`;
}

/** 时间戳 (秒) -> YYYY-MM-DD HH:MM:SS */
function formatFileTime(seconds) {
    const date = new Date((Number(seconds) || 0) * 1000);
    if (Number.isNaN(date.getTime()) || !Number(seconds)) return '未知时间';
    return `${date.getFullYear()}-${pad2(date.getMonth() + 1)}-${pad2(date.getDate())} ` +
        `${pad2(date.getHours())}:${pad2(date.getMinutes())}:${pad2(date.getSeconds())}`;
}

/** 日志目录兜底: 空值回默认的 "./" */
function normalizeLogDir(value) {
    const dir = String(value === undefined || value === null ? '' : value).trim();
    return dir === '' ? DEFAULT_LOG_DIR : dir;
}

/** Web 端口是否合法 (与后端解析 web_port 的区间一致) */
function isValidWebPort(value) {
    const port = Number(value);
    return Number.isInteger(port) && port >= 1 && port <= 65535;
}

/** 空的时间窗口编辑项 */
function defaultEditTimeWindow() {
    return { startDay: '', startTime: '', endDay: '', endTime: '' };
}

/** 后端格式 ["mon 08:00" ...] -> 编辑格式 */
function timeWindowsToEdit(windows) {
    return (windows || []).map(window => {
        const startParts = String(window.start || '').split(' ');
        const endParts = String(window.end || '').split(' ');
        return {
            startDay: startParts[0] || '',
            startTime: startParts[1] || '',
            endDay: endParts[0] || '',
            endTime: endParts[1] || ''
        };
    });
}

/** 编辑格式 -> 后端格式 */
function editToTimeWindows(editWindows) {
    const windows = [];
    (editWindows || []).forEach((edit, index) => {
        const no = index + 1;
        if (!edit.startDay || !edit.startTime || !edit.endDay || !edit.endTime) {
            throw new Error(`第 ${no} 个时间段未填写完整, 请补全或删除该时间段`);
        }
        const startDay = String(edit.startDay).toLowerCase();
        const endDay = String(edit.endDay).toLowerCase();
        if (!WEEK_DAY_VALUES.includes(startDay) || !WEEK_DAY_VALUES.includes(endDay)) {
            throw new Error(`第 ${no} 个时间段的星期不正确, 应为 周一 ~ 周日`);
        }
        if (!TIME_RE.test(edit.startTime) || !TIME_RE.test(edit.endTime)) {
            throw new Error(`第 ${no} 个时间段的时间格式不正确, 应为 HH:MM (24 小时制)`);
        }

        const start = `${startDay} ${edit.startTime}`;
        const end = `${endDay} ${edit.endTime}`;
        if (start === end) {
            throw new Error(`第 ${no} 个时间段的开始和结束不能相同: ${start}`);
        }

        windows.push({ start, end });
    });

    if (windows.length > MAX_TIME_WINDOWS) {
        throw new Error(`最多支持 ${MAX_TIME_WINDOWS} 个时间段`);
    }
    return windows;
}

/** "mon 08:00" -> "周一 08:00" */
function timeWindowText(value) {
    const parts = String(value || '').split(' ');
    const day = WEEK_DAY_TEXT[parts[0]] || parts[0] || '?';
    return `${day} ${parts[1] || ''}`.trim();
}

class ApiError extends Error {
    constructor(message, status) {
        super(message);
        this.name = 'ApiError';
        this.status = status;
    }
}

const REQUEST_TIMEOUT = 8000;

/** 底层请求: 带超时与可中止, 解析 JSON (空响应视为 {}) */
async function httpJson(url, { method = 'GET', json, timeout = REQUEST_TIMEOUT, signal } = {}) {
    const controller = new AbortController();
    const abort = () => controller.abort();
    if (signal) signal.addEventListener('abort', abort, { once: true });
    const timer = setTimeout(abort, timeout);

    try {
        const response = await fetch(url, {
            method,
            cache: 'no-store',
            headers: json === undefined ? undefined : { 'Content-Type': 'application/json' },
            body: json === undefined ? undefined : JSON.stringify(json),
            signal: controller.signal
        });
        const text = await response.text();
        let data = {};
        try { data = text ? JSON.parse(text) : {}; } catch (_) { data = {}; }
        return { ok: response.ok, status: response.status, text, data, headers: response.headers };
    } catch (error) {
        const timedOut = controller.signal.aborted && !(signal && signal.aborted);
        throw new ApiError(timedOut ? `后端响应超时 (${timeout} ms)` : '无法连接到程序后端, 请确认程序正在运行', 0);
    } finally {
        clearTimeout(timer);
        if (signal) signal.removeEventListener('abort', abort);
    }
}

/** 把接口错误翻译成给用户看的提示; messages 按 HTTP 状态码覆盖默认文案 */
function describeError(error, messages = {}) {
    const known = error && messages[error.status];
    return known || (error && error.message) || '操作失败';
}

/** 接口请求: allow 里的状态码也算正常返回 */
async function request(method, url, options = {}) {
    const { allow = [] } = options;
    const result = await httpJson(url, { ...options, method });
    if (result.ok || allow.includes(result.status)) return result;
    if (result.status === 404) throw new ApiError('后端未提供该接口 (404)', 404);
    throw new ApiError(`请求失败 (HTTP ${result.status})`, result.status);
}

const api = {
    async getConfigs() {
        return (await request('GET', '/api/getConfigs')).data;
    },

    async authStatus() {
        return (await request('GET', '/api/status/auth')).data;
    },

    async onlineStatus() {
        return (await request('GET', '/api/status/online', { allow: [204, 302, 503], timeout: 25000 })).status;
    },

    async saveConfigs(configs) {
        return (await request('POST', '/api/saveConfigs', { json: configs, allow: [204, 400, 500] })).status;
    },

    async applyConfigs() {
        return (await request('POST', '/api/applyConfigs', { json: { apply: true }, allow: [204, 400, 403, 500, 503] })).status;
    },

    async restartAuth() {
        return (await request('POST', '/api/restartAuth', { json: { restart: true }, allow: [204] })).status;
    },

    async sysInfo() {
        const { status, data } = await request('GET', '/api/status/sys', { allow: [200, 404] });
        return status === 404 ? null : data;
    },

    async logFiles() {
        const { status, data } = await request('GET', '/api/logs', { allow: [200, 404] });
        return status === 404 ? null : data;
    },

    async logContent(name) {
        const { status, text, headers } = await request('GET', '/api/logs?file=' + encodeURIComponent(name), { allow: [200, 404] });
        if (status === 404) throw new ApiError('日志文件不存在或已被轮转', 404);
        return { text, truncated: headers.get('X-Log-Truncated') === '1' };
    }
};

/** 请求重新认证 (导航栏与仪表板共用) */
async function restartAuthRequest() {
    const notify = Alpine.store('notify');
    try {
        await api.restartAuth();
        notify.success('已请求重启, 正在等待程序恢复…');
        await Alpine.store('settings').waitForRestart();
        return true;
    } catch (error) {
        const text = describeError(error, {
            404: '后端未提供重启认证接口',
            503: '程序未加载完成或认证进程不可达'
        });
        notify.error(text.startsWith('后端') || text.startsWith('程序') ? text : '重启认证失败: ' + text);
        return false;
    }
}

document.addEventListener('alpine:init', () => {

    Alpine.store('notify', {
        items: [],
        seq: 0,

        push(text, type = 'info', timeout = 3500) {
            const id = ++this.seq;
            // 类名必须在这里写成完整字符串, 否则 Tailwind 打包时扫不到, 提示就没有颜色
            this.items.push({ id, text, cls: TOAST_CLASS[type] || TOAST_CLASS.info });
            setTimeout(() => this.remove(id), timeout);
        },

        info(text) { this.push(text, 'info'); },
        success(text) { this.push(text, 'success'); },
        warning(text) { this.push(text, 'warning', 6000); },
        error(text) { this.push(text, 'error', 6000); },

        remove(id) {
            this.items = this.items.filter(item => item.id !== id);
        }
    });

    Alpine.store('main', {
        configs: defaultConfigs(),
        configsLoaded: false,
        configsLoading: false,
        activePanel: 'dashboard',
        menuText: '仪表板',

        init() {
            this.refreshConfigs();
        },

        async refreshConfigs() {
            if (this.configsLoading) return;
            this.configsLoading = true;
            try {
                const data = await api.getConfigs();
                this.configs = normalizeConfigs(data);
                this.configsLoaded = true;
            } catch (error) {
                Alpine.store('notify').error('读取配置失败: ' + error.message);
            } finally {
                this.configsLoading = false;
            }
        },

        /** 当前账号 (桌面端只支持一个账号) */
        get account() {
            return (this.configs.accounts && this.configs.accounts[0]) || null;
        },

        get channelText() {
            const channel = this.account && this.account.channel;
            if (!channel) return '未知';
            return CHANNEL_TEXT[channel] || channel;
        },

        get timeWindowText() {
            const windows = (this.account && this.account.time_windows) || [];
            if (!windows.length) return '不限';
            return `${windows.length} 个时段`;
        },

        /** 切换到指定面板 */
        setPanel(panel, text) {
            this.activePanel = panel;
            this.menuText = text;
            this.updateFavicon(panel);
            this.closeDrawerIfNeeded();
        },

        /** 复位为默认配置 (仅内存, 需要再调用保存) */
        resetConfigs() {
            this.configs = defaultConfigs();
        },

        closeDrawerIfNeeded() {
            if (window.innerWidth < 1024) {
                const drawer = document.getElementById('main-drawer');
                if (drawer) drawer.checked = false;
            }
        },

        updateFavicon(panel) {
            const iconMap = {
                dashboard: 'assets/svg/dashboard.svg',
                settings: 'assets/svg/settings.svg',
                logs: 'assets/svg/logs.svg',
                about: 'assets/svg/about.svg'
            };
            const iconUrl = iconMap[panel] || iconMap.dashboard;
            const link = document.getElementById('favicon');
            if (link) link.href = iconUrl;
        }
    });

    Alpine.store('status', {
        authStatusText: '未知认证状态',
        authErrorText: '',
        authStatusOk: null,
        authBootId: '',
        onlineStatusText: '未知联网状态',
        onlineStatusLevel: 'error',
        lastCheckAt: null,
        timer: null,

        async updateAuthStatus() {
            try {
                const data = await api.authStatus();
                // 查不到认证状态有好几种原因, 由后端给出 reason, 这里换成对应的说法,
                // 不然"配置还没填"也会被显示成"认证进程未运行"
                const reasonText = {
                    config: '配置未就绪',
                    'no-account': '没有可用账号',
                    disabled: '程序已禁用',
                    'not-running': '认证进程未运行'
                };
                // 上一次服务端返回的失败原因 (Error-Code / Sub-Error 翻译过来的话)
                const message = data && data.errorMessage ? data.errorMessage : '';
                const code = data && data.errorCode ? data.errorCode : '';
                const subCode = data && data.subError ? data.subError : '';
                const detail = [code && 'Error-Code: ' + code, subCode && 'Sub-Error: ' + subCode].filter(Boolean).join(', ');
                this.authErrorText = message ? (detail ? message + '（' + detail + '）' : message) : '';

                const bootId = data && data.bootId ? data.bootId : '';

                /** 程序换过一次启动 (被监管者重启、在别的页面点了重启、崩溃后自动重拉): */
                if (this.authBootId && bootId && this.authBootId !== bootId) {
                    this.authBootId = bootId;
                    location.reload();
                    return;
                }
                this.authBootId = bootId;

                const reason = data && data.reason ? data.reason : '';
                if (reason && reasonText[reason]) {
                    this.authStatusOk = null;
                    this.authStatusText = reasonText[reason];
                } else if (data && data.reachable === false) {
                    // 拆分多进程后, 认证进程可能没在跑。
                    // 这时不是"未认证", 而是根本查不到, 要区分开免得误导
                    this.authStatusOk = null;
                    this.authStatusText = '认证进程未运行';
                } else {
                    this.authStatusOk = !!(data && data.status);
                    this.authStatusText = this.authStatusOk ? '已认证' : '未认证';
                }
            } catch (error) {
                this.authStatusOk = null;
                this.authStatusText = '未知认证状态';
            }
        },

        async updateOnlineStatus() {
            try {
                const code = await api.onlineStatus();
                if (code === 204) {
                    this.onlineStatusText = '已连接互联网';
                    this.onlineStatusLevel = 'success';
                } else if (code === 302) {
                    this.onlineStatusText = '互联网需要认证';
                    this.onlineStatusLevel = 'warning';
                } else {
                    this.onlineStatusText = '未连接互联网';
                    this.onlineStatusLevel = 'error';
                }
            } catch (error) {
                this.onlineStatusText = '未知联网状态';
                this.onlineStatusLevel = 'error';
            }
        },

        async refreshNow() {
            await Promise.all([this.updateAuthStatus(), this.updateOnlineStatus()]);
            this.lastCheckAt = new Date();
        },

        start() {
            if (this.timer) return;
            this.refreshNow();
            this.timer = startPolling(() => this.refreshNow(), STATUS_INTERVAL);
        },

        stop() {
            if (!this.timer) return;
            clearInterval(this.timer);
            this.timer = null;
        }
    });

    Alpine.store('settings', {
        saving: false,
        applying: false,

        /** 组装提交给后端的配置 (只包含后端认识的字段, 并纠正类型) */
        buildPayload() {
            const configs = Alpine.store('main').configs || {};
            const payload = {};
            CONFIG_FIELDS.forEach(field => { payload[field.key] = field.out(configs[field.key]); });
            payload.accounts = [normalizeAccount((configs.accounts || [])[0], true)];
            return payload;
        },

        /** 保存前校验, 返回错误说明或 null */
        validate(payload) {
            // 时间窗必须能解析, 否则后端会拒绝整份配置
            try {
                editToTimeWindows(timeWindowsToEdit(payload.accounts[0].time_windows));
            } catch (error) {
                return error.message;
            }

            // 其余字段的校验规则跟着字段表走
            const configs = Alpine.store('main').configs || {};
            for (const field of CONFIG_FIELDS) {
                const invalid = field.check && field.check(configs[field.key]);
                if (invalid) return invalid;
            }
            return null;
        },

        async saveConfigs() {
            if (this.saving) return false;
            const notify = Alpine.store('notify');
            const payload = this.buildPayload();
            // 记下保存前的端口与外部访问开关, 用来判断这次改动要不要重启才生效
            const before = Alpine.store('main').configs || {};

            const invalid = this.validate(payload);
            if (invalid) {
                notify.error('配置校验失败: ' + invalid);
                return false;
            }

            this.saving = true;
            try {
                const code = await api.saveConfigs(payload);
                if (code === 204) {
                    notify.success('配置已保存');
                    await Alpine.store('main').refreshConfigs();
                    if (before.web_port !== payload.web_port ||
                        !!before.web_external_acc !== payload.web_external_acc) {
                        notify.warning(`Web 端口与外部访问开关要重启程序才会生效, 当前仍在 ${location.host} 上`);
                    }
                    if (!payload.accounts[0].username || !payload.accounts[0].password) {
                        notify.warning('账号或密码为空, 程序无法完成认证');
                    }
                    return true;
                }
                notify.error(code === 400 ? '保存失败: 配置内容为空' : '保存失败: 配置非法或无法写入配置文件');
                return false;
            } catch (error) {
                notify.error('保存失败: ' + describeError(error, { 400: '配置内容为空' }));
                return false;
            } finally {
                this.saving = false;
            }
        },

        async applyConfigs() {
            const notify = Alpine.store('notify');
            try {
                const code = await api.applyConfigs();
                if (code === 204) {
                    notify.info('配置已提交, 程序正在重启…');
                    this.waitForRestart();
                    return true;
                }
                return false;
            } catch (error) {
                notify.error('应用失败: ' + describeError(error, {
                    404: '后端未提供应用接口',
                    400: '请求内容为空',
                    403: '没有线程正在认证',
                    500: 'JSON 内容错误'
                }));
                return false;
            }
        },

        async saveAndApply() {
            if (this.applying) return;
            this.applying = true;
            try {
                const saved = await this.saveConfigs();
                if (saved) await this.applyConfigs();
            } finally {
                this.applying = false;
            }
        },

        async resetConfigs() {
            const notify = Alpine.store('notify');
            Alpine.store('main').resetConfigs();
            const ok = await this.saveConfigs();
            if (ok) {
                // 和 LuCI 保持一致: 复位只写文件不重启, 提示完刷新页面, 让用户对着默认值填账号
                notify.success('配置已复位到默认值, 页面即将刷新, 请重新填写账号');
                setTimeout(() => location.reload(), 2000);
            }
        },

        /** 等程序重启完成, 然后自动刷新页面。 */
        async waitForRestart() {
            const notify = Alpine.store('notify');
            const status = Alpine.store('status');
            const before = status.authBootId;
            let offline = false;

            for (let i = 0; i < 40; i++) {
                await sleep(1000);
                try {
                    const data = await api.authStatus();
                    const now = data && data.bootId ? data.bootId : '';
                    if (now && before && now !== before) {
                        notify.success('程序已重启, 正在刷新页面…');
                        location.reload();
                        return;
                    }
                    if (offline && now) {
                        // 曾经掉线过又回来了, 但没拿到可比的旧值: 直接刷新
                        notify.success('程序已重启, 正在刷新页面…');
                        location.reload();
                        return;
                    }
                } catch (error) {
                    // 重启期间连不上是正常的, 继续等
                    offline = true;
                }
            }

            notify.warning('等待重启信号超时, 将刷新页面重新获取状态');
            location.reload();
        }
    });

    Alpine.data('editConfigs', () => ({
        accounts: [],
        timeWindows: [],
        error: '',
        /** 高级选项是否展开 (只是界面状态, 不写进配置) */
        showAdvanced: false,

        init() {
            this.getAccounts();
        },

        /** 从全局配置拷贝一份, 避免未保存的修改直接影响界面 */
        getAccounts() {
            const original = Alpine.store('main').configs.accounts || [];
            this.accounts = JSON.parse(JSON.stringify(original));
            if (!this.accounts.length) {
                this.accounts = defaultConfigs().accounts;
            }
            this.accounts.forEach(account => {
                if (!Array.isArray(account.time_windows)) account.time_windows = [];
            });
            this.error = '';
            this.syncTimeWindows();
        },

        get account() {
            return this.accounts[0] || null;
        },

        syncTimeWindows() {
            this.timeWindows = timeWindowsToEdit(this.account && this.account.time_windows);
        },

        addTimeWindow() {
            if (this.timeWindows.length >= MAX_TIME_WINDOWS) {
                this.error = `最多支持 ${MAX_TIME_WINDOWS} 个时间段`;
                return;
            }
            this.error = '';
            this.timeWindows.push(defaultEditTimeWindow());
        },

        /** 探测目标总路数: 添加按钮用它判断是否已到 14 路上限 */
        probeIpTotal() {
            const configs = Alpine.store('main').configs || {};
            return probeHostsOut(configs.probe_hosts).reduce((sum, item) => sum + item.ips.length, 0);
        },

        removeTimeWindow(index) {
            this.timeWindows.splice(index, 1);
            this.error = '';
        },

        /** 只写入全局配置, 由"保存"按钮决定何时落盘 */
        saveAccounts() {
            let windows;
            try {
                windows = editToTimeWindows(this.timeWindows);
            } catch (error) {
                this.error = error.message;
                Alpine.store('notify').error(error.message);
                return false;
            }

            if (!this.account.username) {
                this.error = '账号不能为空';
                return false;
            }

            this.account.time_windows = windows;
            Alpine.store('main').configs.accounts = JSON.parse(JSON.stringify(this.accounts));
            this.error = '';
            Alpine.store('notify').success('账号信息已更新, 别忘了点击保存');
            return true;
        },

        openModal() {
            this.getAccounts();
            this.toggleModal('setModal', 'open');
        },

        toggleModal(modalName, action) {
            const modal = this.$refs[modalName];
            if (!modal) return;
            if (action === 'open') modal.showModal();
            else if (action === 'close') modal.close();
        }
    }));

    Alpine.data('restartAuth', () => ({
        busy: false,

        open() {
            const modal = this.$refs.restartModal;
            if (modal) modal.showModal();
        },

        async confirm() {
            const modal = this.$refs.restartModal;
            if (modal) modal.close();

            this.busy = true;
            try {
                await restartAuthRequest();
            } finally {
                this.busy = false;
            }
        }
    }));

    Alpine.data('dashboard', () => ({
        sys: null,
        sysAvailable: true,
        sysLoading: false,
        sysLoadedAt: null,
        uptimeBase: null,
        now: Date.now(),
        tickTimer: null,
        restarting: false,

        init() {
            this.refreshSys();
            this.tickTimer = setInterval(() => {
                if (document.hidden) return;
                this.now = Date.now();
            }, 1000);
        },

        destroy() {
            if (this.tickTimer) clearInterval(this.tickTimer);
            this.tickTimer = null;
        },

        async restart() {
            if (this.restarting) return;
            this.restarting = true;
            try {
                await restartAuthRequest();
            } finally {
                this.restarting = false;
            }
        },

        async refreshSys() {
            if (this.sysLoading) return;
            this.sysLoading = true;
            try {
                const data = await api.sysInfo();
                this.sys = data;
                this.sysAvailable = !!data;
                if (data && typeof data.uptime_ms === 'number') {
                    this.uptimeBase = Date.now() - data.uptime_ms;
                } else {
                    this.uptimeBase = null;
                }
                this.sysLoadedAt = new Date();
            } catch (error) {
                this.sys = null;
                this.sysAvailable = false;
            } finally {
                this.sysLoading = false;
            }
        },

        get versionText() {
            return (this.sys && this.sys.version) || '未知';
        },

        get uptimeText() {
            if (this.uptimeBase === null) return '未知';
            return formatUptime(this.now - this.uptimeBase);
        },

        get logLevelText() {
            const level = Alpine.store('main').configs.log_lv;
            const map = {
                0: '0 - 关闭',
                1: '1 - 致命',
                2: '2 - 错误',
                3: '3 - 警告',
                4: '4 - 信息',
                5: '5 - 调试',
                6: '6 - 全部'
            };
            return map[level] || `未知 (${level})`;
        },

        get logDirText() {
            return (this.sys && this.sys.log_dir) || '未知';
        },

        get configFileText() {
            return (this.sys && this.sys.config_file) || '未知';
        },

        get statusCheckText() {
            const at = Alpine.store('status').lastCheckAt;
            return at ? formatClock(at) : '尚未检测';
        }
    }));

    Alpine.data('logViewer', () => ({
        files: [],
        selected: '',
        content: '',
        autoscroll: true,
        autoRefresh: true,
        unavailable: false,
        truncated: false,
        loading: false,
        error: '',
        timer: null,
        resizeObserver: null,

        init() {
            this.loadFiles();
            this.timer = startPolling(() => {
                if (this.autoRefresh) this.refresh();
            }, LOG_INTERVAL, 'logs');
            // x-ref 要等子树初始化完才可用, 所以放到 nextTick 里
            this.$nextTick(() => this.observeLayout());
        },

        destroy() {
            if (this.timer) clearInterval(this.timer);
            this.timer = null;
            if (this.resizeObserver) this.resizeObserver.disconnect();
            this.resizeObserver = null;
        },

        /** 日志框尺寸变化 (样式/字体晚就绪) 时也保持置底 */
        observeLayout() {
            if (typeof ResizeObserver !== 'function' || this.resizeObserver) return;
            const pre = this.$refs.logContent;
            if (!pre) return;
            this.resizeObserver = new ResizeObserver(() => this.scrollToBottomIfNeeded());
            this.resizeObserver.observe(pre);
        },

        get selectedFile() {
            return this.files.find(file => file.name === this.selected) || null;
        },

        get selectedInfoText() {
            const file = this.selectedFile;
            if (!file) return '';
            return `${file.name} · ${formatBytes(file.size)} · ${formatFileTime(file.mtime)}` +
                (file.current ? ' · 正在写入' : '');
        },

        async loadFiles() {
            try {
                const data = await api.logFiles();
                if (!data) {
                    this.unavailable = true;
                    this.files = [];
                    this.selected = '';
                    this.content = '';
                    return;
                }
                this.unavailable = false;
                const files = (data.files || []).map(file => ({
                    name: String(file.name || ''),
                    size: Number(file.size) || 0,
                    mtime: Number(file.mtime) || 0,
                    current: !!file.current
                })).filter(file => file.name);
                const changed = files.length !== this.files.length ||
                    files.some((file, index) => file.name !== (this.files[index] || {}).name);

                this.files = files;

                if (!files.length) {
                    this.selected = '';
                    this.content = '';
                    return;
                }
                if (!files.some(file => file.name === this.selected)) {
                    await this.select(files[0].name);
                } else if (changed) {
                    // 列表变化时同步一次内容
                    await this.loadContent();
                }
            } catch (error) {
                this.unavailable = error.status === 404;
                this.error = error.message;
            }
        },

        async select(name) {
            this.selected = name;
            await this.loadContent();
        },

        async loadContent() {
            if (!this.selected) return;
            this.loading = true;
            try {
                const data = await api.logContent(this.selected);
                this.content = data.text;
                this.truncated = data.truncated;
                this.error = '';
                this.scrollToBottomIfNeeded();
            } catch (error) {
                this.content = '';
                this.error = error.message;
            } finally {
                this.loading = false;
            }
        },

        async refresh() {
            await this.loadFiles();
            if (this.selected) await this.loadContent();
        },

        scrollToBottomIfNeeded() {
            if (!this.autoscroll) return;
            const scroll = () => {
                const pre = this.$refs.logContent;
                if (pre && this.autoscroll) pre.scrollTop = pre.scrollHeight;
            };
            this.$nextTick(scroll);
            // 样式/字体可能尚未就绪 (例如开发模式下 Tailwind 浏览器版还在编译), 再补一次
            setTimeout(scroll, 200);
        },

        /** 下载当前日志 */
        download() {
            if (!this.selected) {
                Alpine.store('notify').warning('请先选择日志文件');
                return;
            }
            const blob = new Blob([this.content || ''], { type: 'text/plain;charset=utf-8' });
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = this.selected;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
        }
    }));
});

/** 规范化后端返回的配置, 兼容字段缺失或类型不符的情况 */
function normalizeConfigs(raw) {
    const configs = defaultConfigs();
    if (!raw || typeof raw !== 'object') return configs;

    CONFIG_FIELDS.forEach(field => { configs[field.key] = field.norm(raw[field.key]); });
    configs.accounts = [normalizeAccount((raw.accounts || [])[0])];
    return configs;
}
