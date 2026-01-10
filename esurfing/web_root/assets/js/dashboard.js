let adapterName = [];
let adapterIp = [];

let schoolNetworkConnected = false;

async function getNetworkStatus() {
    await axios({
        method: "get",
        url: "/api/getNetworkStatus",
        timeout: 5000,
        responseType: "json",
        responseEncoding: "utf-8",
    })
        .then(response => {
            const data = response.data;
            const networkStatus = document.getElementById("network-status");
            const connectTime = document.getElementById("connect-time");
            const currentTime = document.getElementById("current-time");
            if (data.isConnected) {
                networkStatus.textContent = "已连接互联网";
                networkStatus.style.color = "green";
                const now = Date.now();
                const connectDuration = now - data.connectTime;
                const day = String(
                    Math.floor(connectDuration / (1000 * 60 * 60 * 24))
                ).padStart(2, "0");
                const hours = String(
                    Math.floor(
                        (connectDuration % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60)
                    )
                ).padStart(2, "0");
                const minutes = String(
                    Math.floor((connectDuration % (1000 * 60 * 60)) / (1000 * 60))
                ).padStart(2, "0");
                const seconds = String(
                    Math.floor((connectDuration % (1000 * 60)) / 1000)
                ).padStart(2, "0");
                connectTime.textContent = `${day} 天 ${hours} 时 ${minutes} 分 ${seconds} 秒`;
            } else if (schoolNetworkConnected) {
                networkStatus.textContent = "已接入校园网, 未连接互联网";
                networkStatus.style.color = "orange";
                connectTime.textContent = "00 天 00 时 00 分 00 秒";
            } else {
                networkStatus.textContent = "未接入校园网, 未连接互联网";
                networkStatus.style.color = "red";
                connectTime.textContent = "00 天 00 时 00 分 00 秒";
            }
            const now = new Date();
            const year = now.getFullYear();
            const month = String(now.getMonth() + 1).padStart(2, "0");
            const day = String(now.getDate()).padStart(2, "0");
            const hours = String(now.getHours()).padStart(2, "0");
            const minutes = String(now.getMinutes()).padStart(2, "0");
            const seconds = String(now.getSeconds()).padStart(2, "0");
            currentTime.textContent = `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`;
        })
        .catch((error) => {
            console.error(error);
        });
}

async function getSoftwareStatus() {
    await axios({
        method: "get",
        url: "/api/getSoftwareStatus",
        timeout: 5000,
        responseType: "json",
        responseEncoding: "utf-8",
    })
        .then(response => {
            const data = response.data;
            const threadRunningStatus = document.querySelectorAll(
                ".thread-running-status"
            );
            let i = 0;
            for (const thread of data.threads) {
                if (thread.threadIsRunning) {
                    threadRunningStatus[i].textContent = "运行中";
                    threadRunningStatus[i].style.color = "green";
                } else {
                    threadRunningStatus[i].textContent = "未运行";
                    threadRunningStatus[i].style.color = "orange";
                }
                i++;
            }
            const now = Date.now();
            const authDuration = now - data.runningTime;
            const day = String(
                Math.floor(authDuration / (1000 * 60 * 60 * 24))
            ).padStart(2, "0");
            const hours = String(
                Math.floor((authDuration % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60))
            ).padStart(2, "0");
            const minutes = String(
                Math.floor((authDuration % (1000 * 60 * 60)) / (1000 * 60))
            ).padStart(2, "0");
            const seconds = String(
                Math.floor((authDuration % (1000 * 60)) / 1000)
            ).padStart(2, "0");
            document.getElementById(
                "running-time"
            ).textContent = `${day} 天 ${hours} 时 ${minutes} 分 ${seconds} 秒`;
            document.getElementById("version").textContent = data.version;
        })
        .catch((error) => {
            console.error(error);
        });
}

const threadSwitch = document.querySelectorAll(".thread-switch");

async function getThreadStatus() {
    await axios({
        method: "get",
        url: "/api/getThreadStatus",
        timeout: 5000,
        responseType: "json",
        responseEncoding: "utf-8",
    }).then(response => {
        const data = response.data;
        const threadAuthStatus = document.querySelectorAll(".thread-auth-status");
        const threadAuthTime = document.querySelectorAll(".thread-auth-time");
        const threadAdapter = document.querySelectorAll(".thread-adapter");
        let i = 0;
        for (const thread of data.threads) {
            if (thread.isAuth) {
                threadAuthStatus[i].textContent = "已认证";
                threadAuthStatus[i].style.color = "green";
            } else {
                threadAuthStatus[i].textContent = "未认证";
                threadAuthStatus[i].style.color = "red";
            }
            if (thread.authTime > 0) {
                const now = Date.now();
                const authDuration = now - thread.authTime;
                const day = String(
                    Math.floor(authDuration / (1000 * 60 * 60 * 24))
                ).padStart(2, "0");
                const hours = String(
                    Math.floor((authDuration % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60))
                ).padStart(2, "0");
                const minutes = String(
                    Math.floor((authDuration % (1000 * 60 * 60)) / (1000 * 60))
                ).padStart(2, "0");
                const seconds = String(
                    Math.floor((authDuration % (1000 * 60)) / 1000)
                ).padStart(2, "0");
                threadAuthTime[i].textContent = `${day} 天 ${hours} 时 ${minutes} 分 ${seconds} 秒`;
            } else {
                threadAuthTime[i].textContent = "00 天 00 时 00 分 00 秒";
            }
            if (thread.userIp != null) {
                for (let j = 0; j < thread.userIp.length; j++) {
                    if (thread.userIp === adapterIp[j]) {
                        threadAdapter[i].textContent = adapterName[j];
                    }
                }
            } else {
                threadAdapter[i].textContent = "无";
            }
            threadSwitch[i].textContent = thread.isRunning ? "关闭" : "开启";
            i++;
        }
    });
}

async function getAdapterInfo() {
    await axios({
        method: "get",
        url: "/api/getAdapterInfo",
        timeout: 5000,
        responseType: "json",
        responseEncoding: "utf-8",
    })
        .then(response => {
            const data = response.data;
            const adapterInfo = document.getElementById("adapter-info");
            adapterInfo.innerHTML = "";
            let count = 1;
            let schoolNetworkCount = 0;
            for (const adapter of data.adapters) {
                adapterName[count] = adapter.name;
                adapterIp[count] = adapter.ip;
                if (adapter.ip.includes(data.school_network_symbol) && data.school_network_symbol !== '') {
                    schoolNetworkCount++;
                }
                if (adapter.ip !== "0.0.0.0") {
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
            schoolNetworkConnected = schoolNetworkCount >= 1;
        })
        .catch((error) => {
            console.error(error);
        });
}

async function manageThread(index) {
    waitSettingsSave();
    await axios({
        method: "post",
        url: "/api/manageThread",
        timeout: 5000,
        headers: {
            "Content-Type": "application/json",
        },
        responseType: "json",
        responseEncoding: "utf-8",
        data: {
            index: index,
        },
    })
        .then(async response => {
            await sleep(5000);
            if (response.status === 204) {
                closeLoadingModal(true);
            } else {
                closeLoadingModal(false);
            }
        })
        .catch((error) => {
            console.error(error);
        });
}

for (let i = 0; i < threadSwitch.length; i++) {
    threadSwitch[i].addEventListener("click", () => {
        manageThread(i);
    })
}

window.addEventListener("load", () => {
    getNetworkStatus();
    getSoftwareStatus();
    getThreadStatus();
    getAdapterInfo();
    setInterval(getNetworkStatus, 1000);
    setInterval(getSoftwareStatus, 1000);
    setInterval(getThreadStatus, 1000);
    setInterval(getAdapterInfo, 5000);
});
