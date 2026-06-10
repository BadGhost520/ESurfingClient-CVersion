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
                    channel: 'phone'
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

        init() {
            const original = Alpine.store('main').configs.accounts;
            this.accounts = JSON.parse(JSON.stringify(original));
        },

        saveAccounts() {
            Alpine.store('main').configs.accounts = JSON.parse(JSON.stringify(this.accounts));
        },

        getAccounts() {
            const original = Alpine.store('main').configs.accounts;
            this.accounts = JSON.parse(JSON.stringify(original));
        },

        toggleModal(modalName, action) {
            const modal = this.$refs[modalName];
            if (!modal) return;
            if (action === 'open') modal.showModal();
            else if (action === 'close') modal.close();
        }
    }));
});