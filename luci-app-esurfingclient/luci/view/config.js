'use strict';
'require ui';
'require request';

return L.view.extend({
    render: function() {
        var view = this;
        if (view._config) {
            return Promise.resolve(view._renderMain());
        }
        return request.get('/cgi-bin/luci/admin/esurfingclient/read_config')
            .then(function(res) {
                if (!res.ok) throw new Error('HTTP ' + res.status);
                return res.json();
            })
            .then(function(config) {
                if (config.error) {
                    return E('p', { class: 'alert-message error' }, '读取失败: ' + config.error);
                }
                view._config = config;
                return view._renderMain();
            })
            .catch(function(err) {
                return E('p', { class: 'alert-message error' }, '请求异常: ' + err.message);
            });
    },

    _renderMain: function() {
        var view = this;
        var config = view._config;
        var accounts = Array.isArray(config.accounts) ? config.accounts : [];

        var logLvNames = ['关闭','致命','错误','警告','信息','调试','全部'];

        var enableCheck = E('input', { type: 'checkbox', id: 'cfg_enabled' });
        enableCheck.checked = Boolean(config.enabled);
        enableCheck.onchange = function() {
            config.enabled = this.checked;
            view.render().then(function(node) {
                var el = document.getElementById('view');
                el.innerHTML = '';
                el.appendChild(node);
            });
        };

        var logLvSelect = E('select', { id: 'cfg_log_lv', class: 'cbi-input-select' });
        for (var i = 0; i <= 6; i++) {
            var opt = E('option', { value: i }, i + ' - ' + logLvNames[i]);
            if (i === (parseInt(config.log_lv,10) || 0)) opt.selected = true;
            logLvSelect.appendChild(opt);
        }
        logLvSelect.onchange = function() {
            config.log_lv = parseInt(this.value, 10);
            view.render().then(function(node) {
                var el = document.getElementById('view');
                el.innerHTML = '';
                el.appendChild(node);
            });
        };

        var headerRow = E('tr', { class: 'cbi-section-table-row' }, [
            E('th', { class: 'cbi-section-table-cell' }, '账号'),
            E('th', { class: 'cbi-section-table-cell' }, '密码'),
            E('th', { class: 'cbi-section-table-cell' }, '通道'),
            E('th', { class: 'cbi-section-table-cell' }, '标记值'),
            E('th', { class: 'cbi-section-table-cell cbi-section-actions', style: 'width:1%; white-space:nowrap;' }, '')
        ]);

        var rows = [headerRow];
        accounts.forEach(function(acc, idx) {
            rows.push(E('tr', { class: 'cbi-section-table-row' }, [
                E('td', { class: 'cbi-section-table-cell' }, acc.username || '(空)'),
                E('td', { class: 'cbi-section-table-cell' }, '••••••'),
                E('td', { class: 'cbi-section-table-cell' }, acc.channel || '(空)'),
                E('td', { class: 'cbi-section-table-cell' }, acc.mark || '(空)'),
                E('td', { class: 'cbi-section-table-cell cbi-section-actions', style: 'width:1%; white-space:nowrap;' }, [
                    E('button', {
                        class: 'cbi-button cbi-button-edit',
                        click: function() { view._openAccountModal('edit', idx); }
                    }, '编辑'),
                    E('button', {
                        class: 'cbi-button cbi-button-remove',
                        click: function() {
                            config.accounts.splice(idx, 1);
                            view.render().then(function(node) {
                                var el = document.getElementById('view');
                                el.innerHTML = '';
                                el.appendChild(node);
                            });
                        }
                    }, '删除')
                ])
            ]));
        });

        var table = E('table', { class: 'cbi-section-table' }, [
            E('thead', {}, headerRow),
            E('tbody', {}, rows.slice(1))
        ]);

        var addBtn = E('button', {
            class: 'cbi-button cbi-button-add',
            click: function() { view._openAccountModal('add'); }
        }, '添加');

        var saveApplyBtn = E('button', {
            class: 'cbi-button cbi-button-apply important',
            click: function() {
                saveApplyBtn.disabled = true;
                saveApplyBtn.textContent = '保存中...';
                view._saveAndApply().finally(function() {
                    saveApplyBtn.disabled = false;
                    saveApplyBtn.textContent = '保存并应用';
                });
            }
        }, '保存并应用');

        return E('div', { class: 'cbi-map' }, [
            E('h2', {}, 'ESurfing 客户端'),
            E('div', { class: 'cbi-map-descr' }, [
                '用于方便地调整 ESurfing 程序的配置文件',
                E('br'),
                '账号密码与原电信认证程序的账号密码相同',
                E('br'),
                '> Powered by BadGhost'
            ]),
            E('div', { class: 'cbi-section', style: 'margin-top: 0px;' }, [
                E('h3', {}, '基本设置'),
                E('div', { class: 'cbi-value' }, [
                    E('label', { class: 'cbi-value-title' }, '启用'),
                    E('div', { class: 'cbi-value-field' }, enableCheck)
                ]),
                E('div', { class: 'cbi-value' }, [
                    E('label', { class: 'cbi-value-title' }, '日志级别'),
                    E('div', { class: 'cbi-value-field' }, logLvSelect)
                ])
            ]),
            E('div', { class: 'cbi-section', style: 'margin-bottom:0px;' }, [
                E('h3', {}, '账号列表 (' + accounts.length + '个)'),
                E('div', { class: 'cbi-section-table-wrapper' }, table),
                E('div', { class: 'cbi-page-actions', style: 'text-align:left; margin-top:10px;' }, addBtn)
            ]),
            E('div', { class: 'cbi-page-actions', style: 'text-align:right; margin-top:0px;' }, saveApplyBtn)
        ]);
    },

    _showNotification: function(message, type) {
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
            setTimeout(function() {
                if (note.parentNode) note.parentNode.removeChild(note);
            }, 300);
        }, 3000);
    },

    _saveAndApply: function() {
        var view = this;
        var enabled = document.getElementById('cfg_enabled').checked;
        var logLv = parseInt(document.getElementById('cfg_log_lv').value, 10);
        var accounts = view._config.accounts || [];

        var newConfig = {
            enabled: enabled,
            log_lv: logLv,
            accounts: accounts
        };

        return request.post('/cgi-bin/luci/admin/esurfingclient/write_config', JSON.stringify(newConfig, null, 2), {
            headers: { 'Content-Type': 'application/json' }
        }).then(function(res) {
            if (!res.ok) throw new Error('写入失败');
            return request.post('/cgi-bin/luci/admin/esurfingclient/restart_service');
        }).then(function(res) {
            if (res.ok) {
                view._showNotification('配置已保存并应用，服务已重启', 'success');
            } else {
                view._showNotification('保存成功，但应用失败', 'error');
            }
        }).catch(function(e) {
            view._showNotification('操作失败: ' + e.message, 'error');
        });
    },

    _openAccountModal: function(mode, idx) {
        var view = this;
        var accounts = view._config.accounts || [];
        var account = (mode === 'edit' && idx !== undefined) ? accounts[idx] : { username: '', password: '', channel: 'phone', mark: '' };

        var passwordVisible = false;
        var pwdInput = E('input', { type: 'password', class: 'cbi-input-text', value: account.password || '', placeholder: '密码' });
        var togglePwdBtn = E('button', {
            class: 'cbi-button cbi-button-neutral', type: 'button',
            style: 'flex-shrink:0; width:60px;',
            click: function(ev) {
                ev.preventDefault();
                passwordVisible = !passwordVisible;
                pwdInput.setAttribute('type', passwordVisible ? 'text' : 'password');
                this.textContent = passwordVisible ? '隐藏' : '显示';
            }
        }, '显示');

        var usernameInput = E('input', { type: 'text', class: 'cbi-input-text', value: account.username || '', placeholder: '用户名', style: 'width:100%;' });

        var channelSelect = E('select', { class: 'cbi-input-select', style: 'width:100%;' });
        ['phone','pc'].forEach(function(val) {
            var opt = E('option', { value: val }, val);
            if (val === account.channel) opt.selected = true;
            channelSelect.appendChild(opt);
        });

        var markInput = E('input', { type: 'text', class: 'cbi-input-text', value: account.mark || '', placeholder: '标记值', style: 'width:100%;' });

        var fieldRow = function(label, widget) {
            return E('div', { style: 'display:flex; align-items:center; margin-bottom:18px;' }, [
                E('label', { style: 'flex-shrink:0; width:80px; text-align:right; margin-right:20px;' }, label),
                E('div', { style: 'width:290px; flex-shrink:0;' }, widget)
            ]);
        };

        var passwordWidget = E('div', { style: 'display:flex; gap:4px; width:100%;' }, [
            E('div', { style: 'flex:1;' }, pwdInput),
            togglePwdBtn
        ]);

        var formBody = E('div', { style: 'padding: 35px 0 0 0;' }, [
            fieldRow('账号', usernameInput),
            fieldRow('密码', passwordWidget),
            fieldRow('通道', channelSelect),
            fieldRow('标记值', markInput)
        ]);

        var contentWrapper = E('div', { style: 'padding-left: 15%;' }, formBody);

        var titleBar = E('div', {
            style: 'background:white; padding:15px 20px; margin:-20px -20px 0 -20px; box-shadow: 0 1px 15px rgba(0,0,0,0.05);'
        }, [ E('span', { style: 'font-weight:bold; font-size:16px; color: #525F7F;' }, mode === 'add' ? '添加账号' : '编辑账号') ]);

        var modalContent = E('div', {
            style: 'background:white; border-radius:4px; padding:20px; width:100%; max-height:calc(100vh - 140px); overflow-y:auto; box-shadow:0 4px 12px rgba(0,0,0,0.3);'
        }, [
            titleBar,
            contentWrapper,
            E('div', { class: 'cbi-page-actions', style: 'margin-top:16px; display:flex; justify-content:flex-end; gap:8px;' }, [
                E('button', {
                    class: 'cbi-button cbi-button-neutral',
                    style: 'background-color: rgb(240, 240, 240); color: #8898AA; border-style:none',
                    click: function() { document.body.removeChild(overlay); }
                }, '关闭'),
                E('button', {
                    class: 'cbi-button cbi-button-save',
                    click: function() {
                        var data = {
                            username: usernameInput.value,
                            password: pwdInput.value,
                            channel: channelSelect.value,
                            mark: markInput.value
                        };
                        if (mode === 'add') {
                            accounts.push(data);
                        } else {
                            accounts[idx] = data;
                        }
                        document.body.removeChild(overlay);
                        view.render().then(function(node) {
                            var el = document.getElementById('view');
                            el.innerHTML = '';
                            el.appendChild(node);
                        });
                    }
                }, '保存')
            ])
        ]);

        var overlay = E('div', {
            style: 'position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(0,0,0,0.5); z-index:1000; padding:70px; box-sizing:border-box; display:flex; justify-content:center; align-items:flex-start; opacity:0; transition:opacity 0.3s ease;'
        }, [ modalContent ]);

        document.body.appendChild(overlay);
        window.setTimeout(function() {
            overlay.style.opacity = '1';
        }, 10);
    },

    handleSave: null,
    handleSaveApply: null,
    handleReset: null
});
