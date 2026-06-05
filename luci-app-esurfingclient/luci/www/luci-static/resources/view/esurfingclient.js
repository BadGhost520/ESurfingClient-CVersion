'use strict';
'require view';
'require fs';
'require ui';
'require rpc';

var callServiceAction = rpc.declare({
    object: 'service',
    method: 'action',
    params: ['name', 'action']
});

var callGetServiceStatus = rpc.declare({
    object: 'service',
    method: 'list',
    params: []
});

return view.extend({
    load: function() {
        var self = this;
        return Promise.all([
            self.loadConfig(),
            self.loadStatus()
        ]);
    },

    render: function() {
        var self = this;

        // Inject custom styles
        var style = document.createElement('style');
        style.textContent = `
            .status-card {
                background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
                border-radius: 12px;
                padding: 20px;
                margin-bottom: 20px;
                color: #fff;
                box-shadow: 0 4px 15px rgba(0,0,0,0.2);
            }
            .status-header {
                display: flex;
                align-items: center;
                justify-content: space-between;
                margin-bottom: 15px;
            }
            .status-title {
                font-size: 18px;
                font-weight: 600;
            }
            .status-indicator {
                display: flex;
                align-items: center;
                gap: 8px;
            }
            .status-dot {
                width: 12px;
                height: 12px;
                border-radius: 50%;
                animation: pulse 2s infinite;
            }
            .status-dot.online { background: #00ff88; box-shadow: 0 0 10px #00ff88; }
            .status-dot.offline { background: #ff4757; box-shadow: 0 0 10px #ff4757; }
            .status-dot.warning { background: #ffa502; box-shadow: 0 0 10px #ffa502; }
            @keyframes pulse {
                0%, 100% { opacity: 1; }
                50% { opacity: 0.5; }
            }
            .status-grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
                gap: 15px;
                margin-top: 15px;
            }
            .status-item {
                text-align: center;
                padding: 10px;
                background: rgba(255,255,255,0.1);
                border-radius: 8px;
            }
            .status-label {
                font-size: 12px;
                color: #aaa;
                margin-bottom: 5px;
            }
            .status-value {
                font-size: 16px;
                font-weight: 600;
                color: #00ff88;
            }
            .status-value.error { color: #ff4757; }
            .control-buttons {
                display: flex;
                gap: 10px;
                margin-top: 15px;
            }
            .control-buttons .cbi-button {
                flex: 1;
                padding: 10px;
                border-radius: 8px;
                font-weight: 600;
                transition: all 0.3s;
            }
            .control-buttons .cbi-button:hover {
                transform: translateY(-2px);
                box-shadow: 0 4px 12px rgba(0,0,0,0.3);
            }
            .btn-start { background: #00ff88; color: #000; }
            .btn-stop { background: #ff4757; color: #fff; }
            .btn-restart { background: #ffa502; color: #000; }
            .diagnostic-card {
                background: #1e1e2e;
                border-radius: 12px;
                padding: 20px;
                margin-bottom: 20px;
                color: #fff;
            }
            .diagnostic-title {
                font-size: 16px;
                font-weight: 600;
                margin-bottom: 15px;
                display: flex;
                align-items: center;
                gap: 8px;
            }
            .diagnostic-grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
                gap: 10px;
            }
            .diagnostic-item {
                background: rgba(255,255,255,0.05);
                border-radius: 8px;
                padding: 12px;
                cursor: pointer;
                transition: all 0.3s;
            }
            .diagnostic-item:hover {
                background: rgba(255,255,255,0.1);
                transform: translateY(-2px);
            }
            .diagnostic-item .icon {
                font-size: 24px;
                margin-bottom: 8px;
            }
            .diagnostic-item .name {
                font-size: 14px;
                font-weight: 500;
            }
            .diagnostic-item .desc {
                font-size: 12px;
                color: #aaa;
                margin-top: 4px;
            }
            .desc {
                font-size: 13px;
                margin-bottom: 5px;
            }
            .modal {
                min-width: 90% !important;
            }
            .log-output {
                background: #0d1117;
                border-radius: 8px;
                padding: 15px;
                font-family: 'Fira Code', monospace;
                font-size: 13px;
                line-height: 1.6;
                color: #c9d1d9;
                max-height: 400px;
                overflow-y: auto;
                white-space: pre-wrap;
                word-break: break-all;
            }
        `;
        document.head.appendChild(style);

        // Tab bar
        var tabBar = E('ul', { class: 'cbi-tabmenu' }, [
            E('li', { class: 'cbi-tab', click: function() { self.switchTab('tab1'); } },
                E('a', {}, '状态')
            ),
            E('li', { class: 'cbi-tab-disabled', click: function() { self.switchTab('tab2'); } },
                E('a', {}, '账号')
            ),
            E('li', { class: 'cbi-tab-disabled', click: function() { self.switchTab('tab3'); } },
                E('a', {}, '日志')
            ),
            E('li', { class: 'cbi-tab-disabled', click: function() { self.switchTab('tab4'); } },
                E('a', {}, '诊断')
            )
        ]);

        self.logs = self.logs || [];

        self.config = self.config || {
            enabled: false,
            log_lv: 0,
            accounts: []
        };

        self.status = self.status || {
            running: false,
            ip: '-',
            acIp: '-',
            uptime: '-',
            nextHeartbeat: '-'
        };

        self.currentTab = 'tab1';

        // Status Panel
        self.status_panel = E('div', { id: 'status_panel', style: 'width: 100%' }, [
            E('div', { class: 'status-card' }, [
                E('div', { class: 'status-header' }, [
                    E('div', { class: 'status-title' }, 'ESurfing 客户端'),
                    E('div', { class: 'status-indicator' }, [
                        E('div', {
                            class: 'status-dot ' + (self.status.running ? 'online' : 'offline'),
                            id: 'status-dot'
                        }),
                        E('span', { id: 'status-text' }, self.status.running ? '运行中' : '已停止')
                    ])
                ]),
                E('div', { class: 'status-grid' }, [
                    E('div', { class: 'status-item' }, [
                        E('div', { class: 'status-label' }, '客户端IP'),
                        E('div', { class: 'status-value', id: 'client-ip' }, self.status.ip)
                    ]),
                    E('div', { class: 'status-item' }, [
                        E('div', { class: 'status-label' }, 'AC服务器'),
                        E('div', { class: 'status-value', id: 'ac-ip' }, self.status.acIp)
                    ]),
                    E('div', { class: 'status-item' }, [
                        E('div', { class: 'status-label' }, '运行时长'),
                        E('div', { class: 'status-value', id: 'uptime' }, self.status.uptime)
                    ]),
                    E('div', { class: 'status-item' }, [
                        E('div', { class: 'status-label' }, '下次心跳'),
                        E('div', { class: 'status-value', id: 'next-heartbeat' }, self.status.nextHeartbeat)
                    ])
                ]),
                E('div', { class: 'control-buttons' }, [
                    E('button', {
                        class: 'cbi-button btn-start',
                        click: function() { self.controlService('start'); }
                    }, '▶ 启动'),
                    E('button', {
                        class: 'cbi-button btn-stop',
                        click: function() { self.controlService('stop'); }
                    }, '⏹ 停止'),
                    E('button', {
                        class: 'cbi-button btn-restart',
                        click: function() { self.controlService('restart'); }
                    }, '🔄 重启')
                ])
            ])
        ]);

        // Accounts Panel
        self.tableBody = self.renderTable();

        self.accounts_panel = E('div', { id: 'accounts_panel', style: 'display: none' }, [
            E('h3', { style: 'margin-top: 0;' }, '账号设置'),
            E('table', { class: 'table cbi-section-table' }, [
                E('thead', { class: 'thead' }, [
                    E('tr', { class: 'tr cbi-section-table-titles' }, [
                        E('th', { class: 'th' }, '账号'),
                        E('th', { class: 'th' }, '密码'),
                        E('th', { class: 'th' }, '通道'),
                        E('th', { class: 'th' }, '标记值'),
                        E('th', { class: 'th', style: 'width: 135px' }, '')
                    ])
                ]),
                self.tableBody
            ]),
            E('button', { class: 'cbi-button cbi-button-add', style: 'margin: 10px; margin-left: 15px;', click: function() {
                self.showModal(self.config.accounts.length);
            } }, '添加')
        ]);

        // Log Panel
        self.logs_selected = self.renderLogs();

        self.log_panel = E('div', { id: 'log_panel', style: 'display: none' }, [
            E('h3', { style: 'margin-top: 0;' }, '日志查看'),
            E('div', { class: 'cbi-value' }, [
                E('div', { class: 'cbi-value-field' }, self.logs_selected)
            ]),
            E('div', { class: 'log-output', id: 'log_content' }, '暂无日志, 或客户端未启动')
        ]);

        // Diagnostic Panel
        self.diagnostic_panel = E('div', { id: 'diagnostic_panel', style: 'display: none' }, [
            E('h3', { style: 'margin-top: 0;' }, '网络诊断'),
            E('div', { class: 'diagnostic-grid' }, [
                E('div', { class: 'diagnostic-item', click: function() { self.runDiagnostic('ping'); } }, [
                    E('div', { class: 'icon' }, '📡'),
                    E('div', { class: 'name' }, 'Ping测试'),
                    E('div', { class: 'desc' }, '检测网络连通性')
                ]),
                E('div', { class: 'diagnostic-item', click: function() { self.runDiagnostic('dns'); } }, [
                    E('div', { class: 'icon' }, '🌐'),
                    E('div', { class: 'name' }, 'DNS解析'),
                    E('div', { class: 'desc' }, '检查DNS解析状态')
                ]),
                E('div', { class: 'diagnostic-item', click: function() { self.runDiagnostic('auth'); } }, [
                    E('div', { class: 'icon' }, '🔐'),
                    E('div', { class: 'name' }, '认证状态'),
                    E('div', { class: 'desc' }, '检查认证服务器状态')
                ]),
                E('div', { class: 'diagnostic-item', click: function() { self.runDiagnostic('network'); } }, [
                    E('div', { class: 'icon' }, '🌐'),
                    E('div', { class: 'name' }, '网络信息'),
                    E('div', { class: 'desc' }, '查看网络接口信息')
                ])
            ]),
            E('div', { style: 'margin-top: 15px;' }, [
                E('div', { class: 'diagnostic-title' }, '诊断结果'),
                E('div', { class: 'log-output', id: 'diagnostic_output' }, '点击上方按钮运行诊断...')
            ])
        ]);

        return [
            E('div', { class: 'cbi-section' }, [
                E('h2', 'ESurfing 客户端')
            ]),
            E('div', { style: 'margin-left: 25px;'}, [
                E('p', { class: 'desc' }, '用于方便地调整 ESurfing 程序的配置文件'),
                E('p', { class: 'desc' }, '账号密码与原电信认证程序的账号密码相同'),
                E('p', { class: 'desc' }, '> Powered by BadGhost')
            ]),
            E('div', { class: 'cbi-section' }, [
                tabBar,
                self.status_panel,
                self.accounts_panel,
                self.log_panel,
                self.diagnostic_panel
            ])
        ];
    },

    loadConfig: function() {
        var self = this;
        return fs.read('/etc/config/esurfingclient')
            .then(function(data) {
                try {
                    self.config = JSON.parse(data);
                } catch(e) {
                    self.config = {};
                }
                return self.config;
            })
            .catch(function() {
                self.config = {
                    enabled: false,
                    log_lv: 0,
                    accounts: []
                };
                return self.config;
            });
    },

    loadStatus: function() {
        var self = this;
        // Simulate loading status from the service
        return callGetServiceStatus('esurfingclient')
            .then(function(status) {
                self.status = {
                    running: status && status['esurfingclient'] && status['esurfingclient'].running,
                    ip: self.status.ip || '-',
                    acIp: self.status.acIp || '-',
                    uptime: self.status.uptime || '-',
                    nextHeartbeat: self.status.nextHeartbeat || '-'
                };
                return self.status;
            })
            .catch(function() {
                self.status = {
                    running: false,
                    ip: '-',
                    acIp: '-',
                    uptime: '-',
                    nextHeartbeat: '-'
                };
                return self.status;
            });
    },

    saveConfig: function() {
        var self = this;
        return fs.write('/etc/config/esurfingclient', JSON.stringify(self.config, null, 2));
    },

    applyConfig: function() {
        return callServiceAction('esurfingclient', 'restart');
    },

    controlService: function(action) {
        var self = this;
        self.showNotification('正在' + (action === 'start' ? '启动' : action === 'stop' ? '停止' : '重启') + '服务...', 'info');
        return callServiceAction('esurfingclient', action)
            .then(function() {
                self.showNotification('服务' + (action === 'start' ? '启动' : action === 'stop' ? '停止' : '重启') + '成功', 'success');
                // Update status indicator
                var dot = document.getElementById('status-dot');
                var text = document.getElementById('status-text');
                if (dot && text) {
                    if (action === 'start' || action === 'restart') {
                        dot.className = 'status-dot online';
                        text.textContent = '运行中';
                    } else {
                        dot.className = 'status-dot offline';
                        text.textContent = '已停止';
                    }
                }
            })
            .catch(function(e) {
                self.showNotification('操作失败: ' + e.message, 'error');
            });
    },

    refreshAccounts: function() {
        var self = this;
        if (self.config.enabled) {
            document.getElementById('enabled').setAttribute('checked', 'checked');
        } else {
            document.getElementById('enabled').removeAttribute('checked');
        }
        var newBody = self.renderTable();
        self.tableBody.parentNode.replaceChild(newBody, self.tableBody);
        self.tableBody = newBody;
    },

    renderTable: function() {
        var self = this;
        self.config.accounts = self.config.accounts || [];
        var rows = self.config.accounts.map(function(account, index) {
            return E('tr', { class: 'tr', 'data-index': index }, [
                E('td', { class: 'td' }, account.username || '(无)'),
                E('td', { class: 'td' }, account.password ? '******' : '(无)'),
                E('td', { class: 'td' }, account.channel),
                E('td', { class: 'td' }, account.mark || '(无)'),
                E('td', { class: 'td' }, [
                    E('button', { class: 'cbi-button cbi-button-edit', click: function() {
                        self.showModal(index);
                    } }, '编辑'),
                    E('button', { class: 'cbi-button cbi-button-remove', click: function() {
                        self.config.accounts.splice(index, 1);
                        self.refreshAccounts();
                    } }, '删除')
                ])
            ]);
        });
        return E('tbody', { class: 'tbody' }, rows);
    },

    showModal: function(index) {
        var self = this;
        var account = {};
        var add_mode = false;
        if (self.config.accounts[index]) {
            account = self.config.accounts[index];
        } else {
            add_mode = true;
            account = {
                username: '',
                password: '',
                channel: 'phone',
                mark: ''
            };
        }

        var modal = L.showModal('编辑账号', [
            E('div', { class: 'cbi-value', style: 'margin-top: 25px;' }, [
                E('label', { class: 'cbi-value-title', style: 'margin-top: 10px;' }, '*账号'),
                E('div', { class: 'cbi-value-field' }, [
                    E('input', { type: 'text', class: 'cbi-input-text', value: account.username, placeholder: '请输入账号', id: 'edit_account' }),
                    E('div', { class: 'cbi-value-description' }, '和官方认证程序的账号相同')
                ])
            ]),
            E('div', { class: 'cbi-value' }, [
                E('label', { class: 'cbi-value-title', style: 'margin-top: 10px;' }, '*密码'),
                E('div', { class: 'cbi-value-field' }, [
                    E('input', { type: 'password', class: 'cbi-input-text', value: account.password, placeholder: '请输入密码', id: 'edit_password' }),
                    E('div', { class: 'cbi-value-description' }, '和官方认证程序的密码相同')
                ])
            ]),
            E('div', { class: 'cbi-value' }, [
                E('label', { class: 'cbi-value-title', style: 'margin-top: 10px;' }, '*通道'),
                E('div', { class: 'cbi-value-field' }, [
                    E('select', { id: 'edit_channel', class: 'cbi-input-select' }, [
                        E('option', { value: 'phone', selected: account.channel === 'phone' ? true : undefined }, 'phone'),
                        E('option', { value: 'pc', selected: account.channel === 'pc' ? true : undefined }, 'pc')
                    ]),
                    E('div', { class: 'cbi-value-description' }, '选择账号的认证通道')
                ])
            ]),
            E('div', { class: 'cbi-value' }, [
                E('label', { class: 'cbi-value-title', style: 'margin-top: 10px;' }, '标记值'),
                E('div', { class: 'cbi-value-field' }, [
                    E('input', { type: 'text', class: 'cbi-input-text', value: account.mark, placeholder: '请输入标记值', id: 'edit_mark' }),
                    E('div', { class: 'cbi-value-description' }, [
                        '可选项, 用于 MWAN 多路区分',
                        E('br'),
                        '十六进制数, 以 0x 开头, 例如 0x100'
                    ])
                ])
            ]),
            E('div', { style: 'text-align: right; margin-top: 20px; padding-top: 10px;' }, [
                E('button', { class: 'cbi-button cbi-button-neutral', click: function() {
                    L.hideModal(modal);
                } }, '关闭'),
                ' ',
                E('button', { class: 'cbi-button cbi-button-apply', click: function() {
                    account.username = document.getElementById('edit_account').value;
                    account.password = document.getElementById('edit_password').value;
                    account.channel = document.getElementById('edit_channel').value;
                    account.mark = document.getElementById('edit_mark').value;

                    if (add_mode) {
                        self.config.accounts.push(account);
                    }

                    self.refreshAccounts();
                    L.hideModal(modal);
                } }, '保存')
            ])
        ]);
    },

    runDiagnostic: function(type) {
        var self = this;
        var output = document.getElementById('diagnostic_output');
        if (!output) return;

        output.textContent = '正在运行诊断...';

        var command;
        switch(type) {
            case 'ping':
                command = 'ping -c 3 114.114.114.114 && echo "---" && ping -c 3 baidu.com';
                break;
            case 'dns':
                command = 'nslookup www.189.cn && echo "---" && cat /etc/resolv.conf';
                break;
            case 'auth':
                command = 'curl -s -o /dev/null -w "HTTP状态码: %{http_code}\\n" http://10.10.10.10 && echo "---" && logread | grep -i esurfing | tail -20';
                break;
            case 'network':
                command = 'ifconfig && echo "---" && ip route';
                break;
            default:
                output.textContent = '未知的诊断类型';
                return;
        }

        // Execute command via rpcd
        var execCommand = rpc.declare({
            object: 'file',
            method: 'exec',
            params: ['command', 'params']
        });

        execCommand('/bin/sh', ['-c', command])
            .then(function(result) {
                output.textContent = result || '命令执行完成，无输出';
            })
            .catch(function(e) {
                output.textContent = '诊断失败: ' + e.message;
            });
    },

    startLogAutoRefresh: function() {
        var self = this;
        if (self.logTimer) clearInterval(self.logTimer);
        self.logTimer = setInterval(function() {
            if (self.currentTab === 'tab3') {
                self.refreshLogs();
                self.loadLogContent();
            }
        }, 500);
    },

    stopLogAutoRefresh: function() {
        var self = this;
        if (self.logTimer) {
            clearInterval(self.logTimer);
            self.logTimer = null;
        }
    },

    loadLogContent: function() {
        var self = this;
        var textarea = document.getElementById('log_content');
        if (!textarea) return;
        fs.read_direct('/var/log/esurfing/logs/' + document.getElementById('log_file').value)
            .then(function(data) {
                textarea.textContent = data || '暂无日志, 或客户端未启动';
            })
            .catch(function() {
                textarea.textContent = '无法读取日志文件';
            });
    },

    refreshLogs: function() {
        var self = this;
        fs.list('/var/log/esurfing/logs')
            .then(function(entries) {
                var new_logs = [];
                for (var i = 0; i < entries.length; i++) {
                    if (entries[i].type === 'file') {
                        new_logs.push(entries[i].name);
                    }
                }
                new_logs.sort((a, b) => b.localeCompare(a));
                if (!self.arraysEqual(self.logs, new_logs)) {
                    self.logs = new_logs;
                    var new_logs_selected = self.renderLogs();
                    self.logs_selected.parentNode.replaceChild(new_logs_selected, self.logs_selected);
                    self.logs_selected = new_logs_selected;
                    document.getElementById('log_file').value = self.logs[0];
                }
            });
    },

    renderLogs: function() {
        var self = this;
        self.logs = self.logs || [];
        var rows = self.logs.map(function(log, index) {
            return E('option', { value: log, selected: self.logs[index] === log ? true : undefined }, log);
        });

        return E('select', {
            id: 'log_file',
            class: 'cbi-input-select',
            value: 'run.log',
            change: function() {
                self.refreshLogs();
            }
        }, rows);
    },

    arraysEqual: function(a, b) {
        if (!a || !b) return a === b;
        if (a.length !== b.length) return false;
        for (var i = 0; i < a.length; i++) {
            if (a[i] !== b[i]) return false;
        }
        return true;
    },

    switchTab: function(tabName) {
        var self = this;
        var tabs = document.querySelectorAll('.cbi-tab, .cbi-tab-disabled');

        self.stopLogAutoRefresh();

        // Hide all panels
        self.status_panel.style.display = 'none';
        self.accounts_panel.style.display = 'none';
        self.log_panel.style.display = 'none';
        self.diagnostic_panel.style.display = 'none';

        // Reset all tabs
        for (var i = 0; i < tabs.length; i++) {
            tabs[i].classList.remove('cbi-tab');
            tabs[i].classList.add('cbi-tab-disabled');
        }

        // Show selected panel and activate tab
        if (tabName === 'tab1') {
            self.currentTab = 'tab1';
            tabs[0].classList.remove('cbi-tab-disabled');
            tabs[0].classList.add('cbi-tab');
            self.status_panel.style.display = 'block';
        } else if (tabName === 'tab2') {
            self.currentTab = 'tab2';
            tabs[1].classList.remove('cbi-tab-disabled');
            tabs[1].classList.add('cbi-tab');
            self.accounts_panel.style.display = 'block';
        } else if (tabName === 'tab3') {
            self.showNotification('正在读取日志', 'info');
            self.currentTab = 'tab3';
            self.refreshLogs();
            self.startLogAutoRefresh();
            tabs[2].classList.remove('cbi-tab-disabled');
            tabs[2].classList.add('cbi-tab');
            self.log_panel.style.display = 'block';
        } else if (tabName === 'tab4') {
            self.currentTab = 'tab4';
            tabs[3].classList.remove('cbi-tab-disabled');
            tabs[3].classList.add('cbi-tab');
            self.diagnostic_panel.style.display = 'block';
        }
    },

    showNotification: function(message, type) {
        if (typeof ui.showNotification === 'function') {
            ui.showNotification(message, type || 'info');
            return;
        }
        var container = document.getElementById('notification-area');
        if (!container) {
            container = document.createElement('div');
            container.id = 'notification-area';
            container.style.cssText = 'position:fixed;top:20px;right:20px;z-index:9999;';
            document.body.appendChild(container);
        }
        var note = document.createElement('div');
        note.className = 'alert-message ' + (type === 'success' ? 'success' : type === 'error' ? 'error' : 'info');
        note.style.cssText = 'margin-bottom:8px;padding:10px 15px;min-width:200px;opacity:0;transition:opacity 0.3s;';
        note.textContent = message;
        container.appendChild(note);
        setTimeout(function() { note.style.opacity = '1'; }, 10);
        setTimeout(function() {
            note.style.opacity = '0';
            setTimeout(function() { if (note.parentNode) note.parentNode.removeChild(note); }, 300);
        }, 3000);
    },

    handleSaveApply: function(ev) {
        var self = this;
        self.showNotification('保存并应用配置中', 'info');
        return self.saveConfig()
            .then(function() {
                return self.applyConfig();
            })
            .then(function() {
                self.showNotification('配置已保存并应用，服务已重启', 'success');
                setTimeout(function() {
                    location.reload();
                }, 2000);
            })
            .catch(function(e) {
                self.showNotification('操作失败: ' + e.message, 'error');
            });
        return false;
    },

    handleSave: function(ev) {
        var self = this;
        self.showNotification('保存配置中', 'info');
        return self.saveConfig()
            .then(function() {
                self.showNotification('配置已保存', 'success');
                setTimeout(function() {
                    location.reload();
                }, 2000);
            })
            .catch(function(e) {
                self.showNotification('配置保存失败: ' + e.message, 'error');
            });
        return false;
    },

    handleReset: function(ev) {
        var self = this;
        var modal = L.showModal('重置配置', [
            E('p', {
                style: 'color: red; margin-top: 30px; font-size: 20px; font-weight: bold; text-align: center;'
            }, '确定要重置配置到默认值吗? 此操作不可逆!'),
            E('div', { style: 'text-align: right; margin-top: 20px; padding-top: 10px;' }, [
                E('button', { class: 'cbi-button cbi-button-neutral', click: function() {
                    L.hideModal(modal);
                } }, '取消'),
                ' ',
                E('button', { class: 'cbi-button cbi-button-reset', click: function() {
                    L.hideModal(modal);
                    self.showNotification('重置配置中', 'info');
                    self.config = {
                        enabled: false,
                        log_lv: 4,
                        accounts: [
                            {
                                username: '',
                                password: '',
                                channel: 'phone',
                                mark: ''
                            }
                        ]
                    };

                    self.refreshAccounts();

                    return self.saveConfig()
                        .then(function() {
                            self.showNotification('配置重置成功, 已重置到默认值', 'success');
                            setTimeout(function() {
                                location.reload();
                            }, 2000);
                        })
                        .catch(function(e) {
                            self.showNotification('配置重置失败: ' + e.message, 'error');
                        });
                } }, '重置')
            ])
        ]);
        return false;
    },
});
