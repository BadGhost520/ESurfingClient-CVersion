async function getNetworkStatus() {
    await axios({
        method: 'get',
        url: '/api/getNetworkStatus',
        timeout: 5000,
        responseType: 'json',
        responseEncoding: 'utf-8'
    })
        .then(response => {
            const data = response.data;
            const connectStatu = document.getElementById('network-status');
            const connectTime = document.getElementById('connect-time');
            const currentTime = document.getElementById('current-time');
            if (data.isConnected) {
                connectStatu.textContent = '已连接';
                connectStatu.style.color = 'green';
                const now = Date.now();
                const connectDuration = now - data.connectTime;
                const day = String(Math.floor(connectDuration / (1000 * 60 * 60 * 24))).padStart(2, '0');
                const hours = String(Math.floor((connectDuration % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60))).padStart(2, '0');
                const minutes = String(Math.floor((connectDuration % (1000 * 60 * 60)) / (1000 * 60))).padStart(2, '0');
                const seconds = String(Math.floor((connectDuration % (1000 * 60)) / 1000)).padStart(2, '0');
                connectTime.textContent = `${day} 天 ${hours} 时 ${minutes} 分 ${seconds} 秒`;
            } else {
                connectStatu.textContent = '未连接';
                connectStatu.style.color = 'red';
                connectTime.textContent = '00 天 00 时 00 分 00 秒';
            }
            const now = new Date();
            const year = now.getFullYear();
            const month = String(now.getMonth() + 1).padStart(2, '0');
            const day = String(now.getDate()).padStart(2, '0');
            const hours = String(now.getHours()).padStart(2, '0');
            const minutes = String(now.getMinutes()).padStart(2, '0');
            const seconds = String(now.getSeconds()).padStart(2, '0');
            currentTime.textContent = `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`;
        })
        .catch(error => {
            console.error(error);
        });
}

async function getSoftwareStatus() {
    await axios({
        method: 'get',
        url: '/api/getSoftwareStatus',
        timeout: 5000,
        responseType: 'json',
        responseEncoding: 'utf-8'
    })
        .then(response => {
            const data = response.data;
            const softwareStatus = document.getElementById('running-status');
            const authStatus = document.getElementById('auth-status');
            const authTime = document.getElementById('auth-time');
            if (data.isRunning) {
                softwareStatus.textContent = '运行中';
                softwareStatus.style.color = 'green';
                if (data.isLogged) {
                    authStatus.textContent = '已认证';
                    authStatus.style.color = 'green';
                    const now = Date.now();
                    const authDuration = now - data.authTime;
                    const day = String(Math.floor(authDuration / (1000 * 60 * 60 * 24))).padStart(2, '0');
                    const hours = String(Math.floor((authDuration % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60))).padStart(2, '0');
                    const minutes = String(Math.floor((authDuration % (1000 * 60 * 60)) / (1000 * 60))).padStart(2, '0');
                    const seconds = String(Math.floor((authDuration % (1000 * 60)) / 1000)).padStart(2, '0');
                    authTime.textContent = `${day} 天 ${hours} 时 ${minutes} 分 ${seconds} 秒`;
                } else {
                    authStatus.textContent = '未认证';
                    authStatus.style.color = 'orange';
                    authTime.textContent = `00 天 00 时 00 分 00 秒`;
                }
            } else {
                softwareStatus.textContent = '未运行';
                softwareStatus.style.color = 'red';
            }
            document.getElementById('version').textContent = data.version;
        })
        .catch(error => {
            console.error(error);
        });
}

async function getAdapterInfo() {
    await axios({
        method: 'get',
        url: '/api/getAdapterInfo',
        timeout: 5000,
        responseType: 'json',
        responseEncoding: 'utf-8'
    })
        .then(response => {
            const data = response.data;
            const adapterInfo = document.getElementById('adapter-info');
            const schoolNetworkStatus = document.getElementById('school-network-status');
            adapterInfo.innerHTML = '';
            let count = 1;
            for (const adapter of data.adapters) {
                if (adapter.name.includes('Virtual') || adapter.name.includes('docker')) continue;
                if (adapter.ip.includes('172.19')) {
                    schoolNetworkStatus.textContent = '已连接';
                    schoolNetworkStatus.style.color = 'green';
                } else {
                    schoolNetworkStatus.textContent = '未连接';
                    schoolNetworkStatus.style.color = 'red';
                }
                if (adapter.ip !== '0.0.0.0') {
                    adapterInfo.innerHTML += `
                        <div>
                            <span>网络适配器 ${count}: ${adapter.name}</span>
                            <span style="color: green;">状态: 已连接</span>
                            <span>IP 地址: ${adapter.ip}</span>
                        </div>
                    `;
                } else {
                    adapterInfo.innerHTML += `
                        <div>
                            <span>网络适配器 ${count}: ${adapter.name}</span>
                            <span style="color: red;">状态: 未连接</span>
                        </div>
                    `;
                }
                count++;
            }
        })
        .catch(error => {
            console.error(error);
        });
}

window.addEventListener('load', () => {
    getNetworkStatus();
    getSoftwareStatus();
    getAdapterInfo();
    setInterval(getNetworkStatus, 1000);
    setInterval(getSoftwareStatus, 1000);
    setInterval(getAdapterInfo, 5000);
});