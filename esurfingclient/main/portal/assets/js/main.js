// 时间窗口辅助函数
function parseTimeWindowLine(line) {
    const match = line.trim().match(/^(mon|tue|wed|thu|fri|sat|sun) (\d{2}):(\d{2})-(mon|tue|wed|thu|fri|sat|sun) (\d{2}):(\d{2})$/i);
    if (!match) return null;

    const startHour = parseInt(match[2], 10);
    const startMinute = parseInt(match[3], 10);
    const endHour = parseInt(match[5], 10);
    const endMinute = parseInt(match[6], 10);
    if (startHour > 23 || startMinute > 59 || endHour > 23 || endMinute > 59) return null;

    const start = match[1].toLowerCase() + ' ' + match[2] + ':' + match[3];
    const end = match[4].toLowerCase() + ' ' + match[5] + ':' + match[6];
    if (start === end) return null;

    return { start: start, end: end };
}

function timeWindowsToText(windows) {
    return (windows || [])
        .map(window => (window.start || '') + '-' + (window.end || ''))
        .join('\n');
}

function timeWindowsFromText(text) {
    const lines = (text || '').split('\n').map(line => line.trim()).filter(Boolean);
    const windows = [];
    for (const line of lines) {
        const window = parseTimeWindowLine(line);
        if (!window) {
            throw new Error('时间窗口格式错误：' + line + '，应为 mon 08:13-mon 23:57');
        }
        windows.push(window);
    }
    return windows;
}

// 用来存储全局变量和函数
document.addEventListener('alpine:init', () => {
    Alpine.store('main', {
        configs: {
            enabled: false,
            log_lv: 0,
            accounts: [
                {
                    username: '',
                    password: '',
                    channel: 'phone',
                    time_windows: []
                }
            ]
        },

        activePanel: 'dashboard',
        menuText: '仪表板',

        init() {
            fetch('/api/getConfigs')
            .then(r => r.json())
            .then(data => {
                this.configs = data;
            })
            .catch(error => {
                // 处理错误，例如显示通知
            });
        },

        closeDrawerIfNeeded() {
            if (window.innerWidth < 1024) {
                document.getElementById('main-drawer').checked = false;
            }
        },

        updateFavicon(panel) {
            const iconMap = {
                dashboard: 'assets/svg/dashboard.svg',
                settings:  'assets/svg/settings.svg',
                logs:      'assets/svg/logs.svg',
                about:     'assets/svg/about.svg'
            };
            const iconUrl = iconMap[panel] || iconMap.dashboard;
            const link = document.getElementById('favicon');
            if (link) {
                link.href = iconUrl;
                const newLink = link.cloneNode(true);
                link.parentNode.replaceChild(newLink, link);
                newLink.id = 'favicon';
            }
        }
    });

    Alpine.store('status', {
        authStatusText: '未知认证状态',
        getAuthStatusTimer: null,
        onlineStatusText: '未知联网状态',
        getOnlineStatusTimer: null,
        updateAuthStatus() {
            const authStatusElement = document.getElementById('authStatus');

            if (this.getAuthStatusTimer) clearInterval(this.getAuthStatusTimer);

            fetch('/api/status/auth')
            .then(r => r.json())
            .then(data => {
                authStatusElement.classList.remove('status-success');
                authStatusElement.classList.remove('status-error');
                if (data.status) {
                this.authStatusText = '已认证';
                authStatusElement.classList.add('status-success');
                } else {
                this.authStatusText = '未认证';
                authStatusElement.classList.add('status-error');
                }
            })
            .catch(error => {
                authStatusElement.classList.remove('status-success');
                authStatusElement.classList.remove('status-error');
                this.authStatusText = '未知认证状态';
                authStatusElement.classList.add('status-error');
            });

            this.getAuthStatusTimer = setInterval(() => {
                this.updateAuthStatus();
            }, 5000);
        },

        updateOnlineStatus() {
            const onlineStatusElement = document.getElementById('onlineStatus');
            onlineStatusElement.classList.remove('status-success');
            onlineStatusElement.classList.remove('status-warning');
            onlineStatusElement.classList.remove('status-error');

            if (this.getOnlineStatusTimer) clearInterval(this.getOnlineStatusTimer);

            fetch('/api/status/online')
            .then(r => {
                if (r.status === 204) {
                    this.onlineStatusText = '已连接互联网';
                    onlineStatusElement.classList.add('status-success');
                } else if (r.status === 302) {
                    this.onlineStatusText = '互联网需要认证';
                    onlineStatusElement.classList.add('status-warning');
                } else if (r.status === 503) {
                    this.onlineStatusText = '未连接互联网';
                    onlineStatusElement.classList.add('status-error');
                } else {
                    this.onlineStatusText = '未知联网状态';
                    onlineStatusElement.classList.add('status-error');
                }
            })
            .catch(error => {
                this.onlineStatusText = '未知联网状态';
                onlineStatusElement.classList.add('status-error');
            });

            this.getOnlineStatusTimer = setInterval(() => {
            this.updateOnlineStatus();
            }, 5000);
        },
    });

    Alpine.store('settings', {
        saveConfigs() {
            fetch('/api/saveConfigs', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify($store.main.configs)
            })
            .then(r => {
                if (r.status === 204) {
                    console.log('保存成功');
                } else if (r.status === 400) {
                    console.log('配置为空');
                } else if (r.status === 500) {
                    console.log('保存失败');
                }
            })
        },

        applyConfigs() {
            fetch('/api/applyConfigs', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ 'apply': true })
            })
            .then(r => {
                if (r.status === 204) {
                    console.log('应用成功');
                } else if (r.status === 400) {
                    console.log('发送内容为空');
                } else if (r.status === 500) {
                    console.log('应用失败');
                }
            })
        }
    }),

    Alpine.data('editConfigs', () => ({
        accounts: [],
        timeWindowsText: '',

        init() {
            const original = Alpine.store('main').configs.accounts;
            this.accounts = JSON.parse(JSON.stringify(original));
            this.accounts.forEach(account => {
                if (account.time_windows === undefined) account.time_windows = [];
            });
            this.syncTimeWindowsText();
        },

        syncTimeWindowsText() {
            this.timeWindowsText = timeWindowsToText(this.accounts[0]?.time_windows);
        },

        saveAccounts() {
            let windows;
            try {
                windows = timeWindowsFromText(this.timeWindowsText);
            } catch (error) {
                alert(error.message);
                return;
            }

            this.accounts[0].time_windows = windows;
            Alpine.store('main').configs.accounts = JSON.parse(JSON.stringify(this.accounts));
        },

        getAccounts() {
            const original = Alpine.store('main').configs.accounts;
            this.accounts = JSON.parse(JSON.stringify(original));
            this.accounts.forEach(account => {
                if (account.time_windows === undefined) account.time_windows = [];
            });
            this.syncTimeWindowsText();
        },

        toggleModal(modalName, action) {
            const modal = this.$refs[modalName];
            if (!modal) return;
            if (action === 'open') modal.showModal();
            else if (action === 'close') modal.close();
        }
    }));
});
