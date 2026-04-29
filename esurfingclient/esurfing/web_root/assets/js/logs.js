const logs = document.getElementById('log');

async function updateLogs() {
    await axios({
        method: 'get',
        url: '/api/updateLogs',
        timeout: 5000,
        responseType: 'text',
        responseEncoding: 'utf-8'
    })
        .then(response => {
            if (response.status === 200) {
                const data = response.data;
                if (data !== '' && data) {
                    logs.innerHTML = data;
                    if (isScrollEnabled) {
                        const scrollBox = logs.parentElement;
                        scrollBox.scrollTop = scrollBox.scrollHeight;
                    }
                }
            } else {
                console.log("无新日志");
            }
        })
        .catch(error => {
            logs.innerHTML = '无法获取日志，Web 服务器可能已关闭\n';
            console.error(error);
        });
}

async function getLogs() {
    await axios({
        method: 'get',
        url: '/api/getLogs',
        timeout: 5000,
        responseType: 'text',
        responseEncoding: 'utf-8'
    })
        .then(response => {
            const data = response.data;
            if (data !== '' && data) {
                logs.innerHTML = data;
                if (isScrollEnabled) {
                    const scrollBox = logs.parentElement;
                    scrollBox.scrollTop = scrollBox.scrollHeight;
                }
            }
        })
        .catch(error => {
            logs.innerHTML = '无法获取日志，Web 服务器可能已关闭\n';
            console.error(error);
        });
}

const logScrollOn = document.getElementById('log-scroll-on');
const logScrollOff = document.getElementById('log-scroll-off');
let isScrollEnabled = true;

logScrollOn.addEventListener('click', () => {
    isScrollEnabled = true;
    if (!logScrollOn.classList.contains('btnon')) logScrollOn.classList.add('btnon');
    logScrollOff.classList.remove('btnon');
});

logScrollOff.addEventListener('click', () => {
    isScrollEnabled = false;
    if (!logScrollOff.classList.contains('btnon')) logScrollOff.classList.add('btnon');
    logScrollOn.classList.remove('btnon');
});

window.addEventListener('load', () => {
    getLogs();
    setInterval(updateLogs, 1000);
});