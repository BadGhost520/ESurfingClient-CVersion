const CHANNEL_TEXT = {
    windows: 'Windows (未实现, Android 替代)',
    linux: 'Linux',
    android: 'Android',
    ios: 'iOS',
    macos: 'MacOS'
};

const CHANNEL_VALUES = ['windows', 'linux', 'android', 'ios', 'macos'];

/**
 * 归一化通道取值
 * 只认 1~5 与 windows / linux / android / ios / macos
 * (iphone / mac / osx 是 LuCI 也保留的历史写法)
 * 其它取值一律按后端默认的 Android 处理
 * @param {*} raw 原始取值
 * @returns {string} windows / linux / android / ios / macos
 */
function normalizeChannel(raw) {
    if (raw === null || raw === undefined) return 'android';

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

/**
 * 归一化超时参数 (秒)
 * 数字输入框清空后会取到 '' 或 NaN, 非正数一律按默认值处理
 * (后端 load_cfg 遇到非法值也会回退到默认值, 这里保持一致)
 * @param {*} raw 原始取值
 * @param {number} fallback 默认值
 * @returns {number} 正整数秒数
 */
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

const DEFAULT_CONN_TIMEOUT = 7;

const DEFAULT_OP_TIMEOUT = 10;

const DEFAULT_WEB_PORT = 8888;

const DEFAULT_LOG_DIR = './';

const MAX_LOG_DIR_LEN = 255;

const STATUS_INTERVAL = 5000;

const LOG_INTERVAL = 5000;

/** 默认配置 (与后端 s_default_cfg 保持一致) */
function defaultConfigs() {
    return {
        enabled: false,
        web_external_acc: false,
        log_lv: 4,
        log_dir: DEFAULT_LOG_DIR,
        conn_timeout: DEFAULT_CONN_TIMEOUT,
        op_timeout: DEFAULT_OP_TIMEOUT,
        web_port: DEFAULT_WEB_PORT,
        accounts: [
            {
                username: '',
                password: '',
                channel: 3,
                time_windows: []
            }
        ]
    };
}

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));

/** 字节数格式化 */
function formatBytes(bytes) {
    const size = Number(bytes);
    if (!Number.isFinite(size) || size < 0) return '未知大小';
    if (size < 1024) return size + ' B';
    if (size < 1024 * 1024) return (size / 1024).toFixed(1) + ' KB';
    return (size / 1024 / 1024).toFixed(2) + ' MB';
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

/**
 * 编辑格式 -> 后端格式
 * 校验规则与后端 parse_time_window 保持一致, 不合法时抛出带中文说明的 Error
 */
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

/** 兼容性: 不支持 AbortSignal.timeout 时退化为无超时 */
function timeoutSignal(ms) {
    if (typeof AbortSignal !== 'undefined' && typeof AbortSignal.timeout === 'function') {
        return AbortSignal.timeout(ms);
    }
    return undefined;
}

/**
 * 统一的请求封装
 * @param {string} method 请求方法
 * @param {string} url 请求地址
 * @param {object} [options] { json, allow, timeout }
 */
async function request(method, url, options = {}) {
    const { json, allow = [], timeout = REQUEST_TIMEOUT } = options;
    let response;
    try {
        response = await fetch(url, {
            method,
            cache: 'no-store',
            headers: json === undefined ? undefined : { 'Content-Type': 'application/json' },
            body: json === undefined ? undefined : JSON.stringify(json),
            signal: timeoutSignal(timeout)
        });
    } catch (error) {
        // 网络层错误: 程序没启动 / 正在重启 / 后端不支持该接口且不返回响应
        if (error && error.name === 'TimeoutError') {
            throw new ApiError(`后端响应超时 (${timeout} ms)`, 0);
        }
        throw new ApiError('无法连接到程序后端, 请确认程序正在运行', 0);
    }

    // 2xx 视为成功; 其它有意义的状态码 (如 302 / 400 / 500 / 503) 由调用方通过 allow 显式接收
    if (response.ok || allow.includes(response.status)) return response;
    if (response.status === 404) throw new ApiError('后端未提供该接口 (404)', 404);
    throw new ApiError(`请求失败 (HTTP ${response.status})`, response.status);
}

const api = {
    async getConfigs() {
        const response = await request('GET', '/api/getConfigs');
        return response.json();
    },

    async authStatus() {
        const response = await request('GET', '/api/status/auth');
        return response.json();
    },

    async onlineStatus() {
        const response = await request('GET', '/api/status/online', {
            allow: [204, 302, 503],
            timeout: 25000
        });
        return response.status;
    },

    async saveConfigs(configs) {
        const response = await request('POST', '/api/saveConfigs', {
            json: configs,
            allow: [204, 400, 500]
        });
        return response.status;
    },

    async applyConfigs() {
        const response = await request('POST', '/api/applyConfigs', {
            json: { apply: true },
            allow: [204, 400, 403, 500, 503]
        });
        return response.status;
    },

    async restartAuth() {
        const response = await request('POST', '/api/restartAuth', {
            json: { restart: true },
            allow: [204]
        });
        return response.status;
    },

    async sysInfo() {
        const response = await request('GET', '/api/status/sys', { allow: [200, 404] });
        return response.status === 404 ? null : response.json();
    },

    async logFiles() {
        const response = await request('GET', '/api/logs', { allow: [200, 404] });
        return response.status === 404 ? null : response.json();
    },

    async logContent(name) {
        const response = await request('GET', '/api/logs?file=' + encodeURIComponent(name), {
            allow: [200, 404]
        });
        if (response.status === 404) throw new ApiError('日志文件不存在或已被轮转', 404);
        return {
            text: await response.text(),
            truncated: response.headers.get('X-Log-Truncated') === '1'
        };
    }
};

/** 请求重新认证 (导航栏与仪表板共用) */
async function restartAuthRequest() {
    const notify = Alpine.store('notify');
    try {
        await api.restartAuth();
        notify.success('已请求重新认证, 认证状态稍后自动刷新');
        setTimeout(() => Alpine.store('status').refreshNow(), 1500);
        return true;
    } catch (error) {
        if (error.status === 404) {
            notify.error('后端未提供重新认证接口');
        } else if (error.status === 403) {
            notify.error('没有正在认证的线程');
        } else if (error.status === 503) {
            notify.error('程序未加载完成');
        } else {
            notify.error('重新认证失败: ' + error.message);
        }
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
        authStatusOk: null,
        onlineStatusText: '未知联网状态',
        onlineStatusLevel: 'error',
        lastCheckAt: null,
        timer: null,

        async updateAuthStatus() {
            try {
                const data = await api.authStatus();
                if (data && data.reachable === false) {
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
            this.timer = setInterval(() => {
                if (document.hidden) return;
                this.refreshNow();
            }, STATUS_INTERVAL);
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
            const account = (configs.accounts && configs.accounts[0]) || {};
            return {
                enabled: !!configs.enabled,
                // 开关与端口都要按类型写回: 后端解析 web_port 时认的是数字
                web_external_acc: !!configs.web_external_acc,
                // 下拉框取到的值是字符串, 这里必须转成数字, 否则后端会当成 0 (关闭日志)
                log_lv: Number(configs.log_lv) || 0,
                log_dir: normalizeLogDir(configs.log_dir),
                // 数字输入框同样可能取到字符串或空值, 统一兜底成正整数秒
                conn_timeout: normalizeTimeout(configs.conn_timeout, DEFAULT_CONN_TIMEOUT),
                op_timeout: normalizeTimeout(configs.op_timeout, DEFAULT_OP_TIMEOUT),
                web_port: Number(configs.web_port),
                accounts: [
                    {
                        username: String(account.username || ''),
                        password: String(account.password || ''),
                        // 通道统一成 LuCI 的写法 (windows/linux/android/ios/macos)
                        channel: normalizeChannel(account.channel),
                        time_windows: (account.time_windows || []).map(window => ({
                            start: String(window.start || ''),
                            end: String(window.end || '')
                        }))
                    }
                ]
            };
        },

        /** 保存前校验, 返回错误说明或 null */
        validate(payload) {
            const account = payload.accounts[0];
            try {
                // 时间窗口必须合法, 否则后端会拒绝整个配置
                editToTimeWindows(timeWindowsToEdit(account.time_windows));
            } catch (error) {
                return error.message;
            }
            const configs = Alpine.store('main').configs || {};
            if (!isValidWebPort(configs.web_port)) {
                return `Web 服务端口应为 1 - 65535 的整数 (当前: ${configs.web_port})`;
            }
            if (normalizeLogDir(configs.log_dir).length > MAX_LOG_DIR_LEN) {
                return `日志目录过长 (最多 ${MAX_LOG_DIR_LEN} 个字符)`;
            }
            if (!account.username || !account.password) {
                // 允许保存空账号 (复位后就是这样), 仅提示
                return null;
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
                if (code === 400) {
                    notify.error('保存失败: 配置内容为空');
                } else {
                    notify.error('保存失败: 配置非法或无法写入配置文件');
                }
                return false;
            } catch (error) {
                notify.error('保存失败: ' + error.message);
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
                    notify.info('已请求应用配置, 程序即将重启');
                    this.waitForRestart();
                    return true;
                }
                return false;
            } catch (error) {
                if (error.status === 404) {
                    notify.error('后端未提供应用接口');
                } else if (error.status === 400) {
                    notify.error('应用失败: 请求内容为空');
                } else if (error.status === 403) {
                    notify.error('应用失败: 没有线程正在认证');
                } else if (error.status === 500) {
                    notify.error('应用失败: JSON 内容错误');
                }
                else {
                    notify.error('应用失败: ' + error.message);
                }
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
            if (ok) notify.info('配置已复位到默认值, 需要重新填写账号后重启程序');
        },

        async waitForRestart() {
            const notify = Alpine.store('notify');
            let offline = false;
            for (let i = 0; i < 15; i++) {
                await sleep(2000);
                try {
                    await api.authStatus();
                    if (offline) {
                        notify.success('程序已重启完成');
                        Alpine.store('status').refreshNow();
                        return;
                    }
                } catch (error) {
                    offline = true;
                }
            }
            if (!offline) {
                notify.info('配置已应用, 程序未重启 (若修改了账号信息请手动重启程序)');
            } else {
                notify.warning('等待程序重启超时, 请检查程序是否正常运行');
            }
        }
    });


    Alpine.data('editConfigs', () => ({
        accounts: [],
        timeWindows: [],
        error: '',

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
            this.timer = setInterval(() => {
                if (document.hidden) return;
                if (Alpine.store('main').activePanel !== 'logs') return;
                if (!this.autoRefresh) return;
                this.refresh();
            }, LOG_INTERVAL);
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

    configs.enabled = !!raw.enabled;
    configs.web_external_acc = !!raw.web_external_acc;

    const level = Number(raw.log_lv);
    configs.log_lv = Number.isFinite(level) ? Math.min(Math.max(level, 0), 6) : 4;

    // 日志目录: 后端给的是配置里的原文, 空值按默认的 "./" 显示
    configs.log_dir = normalizeLogDir(raw.log_dir);

    // 后端可能返回缺失或非法的超时参数, 这里同样兜底, 避免输入框显示空值
    configs.conn_timeout = normalizeTimeout(raw.conn_timeout, DEFAULT_CONN_TIMEOUT);
    configs.op_timeout = normalizeTimeout(raw.op_timeout, DEFAULT_OP_TIMEOUT);

    // 端口非法时先用默认值填上输入框; 真要保存时 validate 会拦下来
    configs.web_port = isValidWebPort(raw.web_port) ? Number(raw.web_port) : DEFAULT_WEB_PORT;

    const accounts = Array.isArray(raw.accounts) ? raw.accounts : [];
    const account = accounts[0] || {};
    configs.accounts = [
        {
            username: String(account.username || ''),
            password: String(account.password || ''),
            // 后端可能返回 1~5 或 windows/linux/android/ios/macos
            channel: normalizeChannel(account.channel),
            time_windows: (Array.isArray(account.time_windows) ? account.time_windows : [])
                .map(window => ({
                    start: String((window && window.start) || ''),
                    end: String((window && window.end) || '')
                }))
                .filter(window => window.start && window.end)
        }
    ];
    return configs;
}
