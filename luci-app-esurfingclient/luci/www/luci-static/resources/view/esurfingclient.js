'use strict';
'require view';
'require fs';
'require ui';

return view.extend({
    load: function() {
        var self = this;

        return self.loadConfig();
    },
    
    render: function() {
        var self = this;

        var style = document.createElement('style');
        style.textContent = `
            .desc {
                font-size: 13px;
                margin-bottom: 5px;
            }

            .modal {
                min-width: 90% !important;
            }
        `;
        document.head.appendChild(style);
        
        var tabBar = E('ul', { class: 'cbi-tabmenu' }, [
            E('li', { class: 'cbi-tab', click: function() { self.switchTab('tab1'); } }, 
                E('a', {}, '常规')
            ),
            E('li', { class: 'cbi-tab-disabled', click: function() { self.switchTab('tab2'); } }, 
                E('a', {}, '账号')
            ),
            E('li', { class: 'cbi-tab-disabled', click: function() { self.switchTab('tab3'); } }, 
                E('a', {}, '日志')
            )
        ]);

        self.logs = self.logs || [];

        self.config = self.config || {
            enabled: false,
            log_lv: 0,
            accounts: []
        };
        
        self.currentTab = 'tab1';

        self.basic_panel = E('div', { id: 'basic_panel', style: 'width: 100%' }, [
            E('h3', { style: 'margin-top: 0;' }, '基本设置'),
            E('div', { class: 'cbi-value' }, [
                E('label', { class: 'cbi-value-title' }, '启用'),
                E('div', { class: 'cbi-value-field' }, [
                    E('input', {
                        type: 'checkbox',
                        id: 'enabled',
                        checked: self.config.enabled ? true : undefined,
                        change: function(ev) {
                            self.config.enabled = ev.target.checked;
                        }
                    }),
                    E('div', { class: 'cbi-value-description' }, '启用 ESurfing 客户端')
                ])
            ]),
            E('div', { class: 'cbi-value' }, [
                E('label', { class: 'cbi-value-title' }, '日志等级'),
                E('div', { class: 'cbi-value-field' }, [
                    E('select', {
                        id: 'log_lv',
                        class: 'cbi-input-select',
                        change: function(ev) {
                            self.config.log_lv = parseInt(ev.target.value);
                        }
                    }, [
                        E('option', { value: 0, selected: self.config.log_lv === 0 ? true : undefined }, '0 - 关闭'),
                        E('option', { value: 1, selected: self.config.log_lv === 1 ? true : undefined }, '1 - 致命'),
                        E('option', { value: 2, selected: self.config.log_lv === 2 ? true : undefined }, '2 - 错误'),
                        E('option', { value: 3, selected: self.config.log_lv === 3 ? true : undefined }, '3 - 警告'),
                        E('option', { value: 4, selected: self.config.log_lv === 4 ? true : undefined }, '4 - 信息'),
                        E('option', { value: 5, selected: self.config.log_lv === 5 ? true : undefined }, '5 - 调试'),
                        E('option', { value: 6, selected: self.config.log_lv === 6 ? true : undefined }, '6 - 全部')
                    ]),
                    E('div', { class: 'cbi-value-description' }, '日志的详细程度')
                ])
            ])
        ]);

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
        
        self.logs_selected = self.renderLogs();

        self.log_panel = E('div', { id: 'log_panel', style: 'display: none' }, [
            E('h3', { style: 'margin-top: 0;' }, '日志查看'),
            E('div', { class: 'cbi-value' }, [
                E('div', { class: 'cbi-value-field' }, self.logs_selected)
            ]),
            E('div', { class: 'cbi-section' }, [
                E('textarea', {
                    id: 'log_content',
                    class: 'cbi-input-textarea',
                    readonly: true,
                    rows: 20,
                    style: 'font-family: monospace; width: 100%;'
                }, '暂无日志, 或客户端未启动')
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
                self.basic_panel,
                self.accounts_panel,
                self.log_panel
            ])
        ];
    },

    loadConfig: function() {
        var self = this;

        self.showNotification('正在读取配置文件', 'info');

        return fs.read('/etc/config/esurfingclient')
            .then(function(data) {
                try {
                    self.config = JSON.parse(data);
                } catch(e) {
                    self.config = {};
                }
                self.showNotification('读取配置文件成功', 'success');
                return self.config;
            })
            .catch(function() {
                self.config = {
                    enabled: false,
                    log_lv: 0,
                    accounts: [
                        {
                            username: '加载失败',
                            password: '加载失败',
                            channel: '加载失败',
                            mark: '加载失败'
                        }
                    ]
                };
                self.showNotification('配置文件读取失败', 'error');
                return self.config;
            });
    },

    saveConfig: function() {
        var self = this;

        return fs.write('/etc/config/esurfingclient', JSON.stringify(self.config, null, 2));
    },

    applyConfig: function() {
        var restartCommand = L.rpc.declare({
            object: 'file',
            method: 'exec',
            params: ['command', 'params']
        });
        return restartCommand('/etc/init.d/esurfingclient', ['restart']);
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

    startLogAutoRefresh: function() {
        var self = this;

        if (self.logTimer) clearInterval(self.logTimer);
        self.logTimer = setInterval(function() {
            if (self.currentTab === 'tab3') {
                self.refreshLogs();
                self.loadLogContent();
            }
        }, 5000);
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
            textarea.value = data || '暂无日志, 或客户端未启动';
        })
        .catch(function() {
            textarea.value = '无法读取日志文件';
            self.showNotification('日志文件读取失败', 'error');
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
        
        if (tabName === 'tab1') {
            self.currentTab = 'tab1';
            tabs[0].classList.remove('cbi-tab-disabled');
            tabs[0].classList.add('cbi-tab');

            tabs[1].classList.remove('cbi-tab');
            tabs[1].classList.add('cbi-tab-disabled');
            tabs[2].classList.remove('cbi-tab');
            tabs[2].classList.add('cbi-tab-disabled');
            
            self.basic_panel.style.display = 'block';
            self.accounts_panel.style.display = 'none';
            self.log_panel.style.display = 'none';
        } else if (tabName === 'tab2') {
            self.currentTab = 'tab2';
            tabs[1].classList.remove('cbi-tab-disabled');
            tabs[1].classList.add('cbi-tab');

            tabs[0].classList.remove('cbi-tab');
            tabs[0].classList.add('cbi-tab-disabled');
            tabs[2].classList.remove('cbi-tab');
            tabs[2].classList.add('cbi-tab-disabled');

            self.basic_panel.style.display = 'none';
            self.accounts_panel.style.display = 'block';
            self.log_panel.style.display = 'none';
        } else if (tabName === 'tab3') {
            self.showNotification('正在读取日志', 'info');
            self.currentTab = 'tab3';
            self.refreshLogs();
            self.startLogAutoRefresh();
            tabs[2].classList.remove('cbi-tab-disabled');
            tabs[2].classList.add('cbi-tab');

            tabs[0].classList.remove('cbi-tab');
            tabs[0].classList.add('cbi-tab-disabled');
            tabs[1].classList.remove('cbi-tab');
            tabs[1].classList.add('cbi-tab-disabled');

            self.basic_panel.style.display = 'none';
            self.accounts_panel.style.display = 'none';
            self.log_panel.style.display = 'block';
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
        if (type === 'info') {
            note.textContent = message + ' ~(￣▽￣~)~';
        } else if (type === 'success') {
            note.textContent = message + ' ≧∇≦';
        } else if (type === 'error') {
            note.textContent = message + ' >_<';
        }
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
                self.showNotification('配置已保存并应用，服务已重启 ', 'success');
                setTimeout(function() {
                    location.reload();
                }, 2000);
            })
            .catch(function(e) {
                self.showNotification('操作失败: ' + e.message, 'error');
                setTimeout(function() {
                    location.reload();
                }, 2000);
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
                setTimeout(function() {
                    location.reload();
                }, 2000);
            });

        return false;
    },

    handleReset: function(ev) {
        var self = this;

        var modal = L.showModal('重置配置', [
            E('p', {
                style: 'color: red; margin-top: 30px; font-size: 20px; font-weight: bold; text-align: center;'
            }, '确定要重置配置到默认值吗? ∑(°Д°) 此操作不可逆!'),
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
                            setTimeout(function() {
                                location.reload();
                            }, 2000);
                        });
                } }, '重置'),
                E('div', { style: 'text-align: right; font-size: 14px; color: #969696;' }, '真的不可逆喔...')
            ])
        ]);

        return false;
    },
});