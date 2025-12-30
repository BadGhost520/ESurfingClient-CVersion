async function getLicense() {
    await axios({
        method: 'get',
        url: '/license/LICENSE',
        timeout: 5000,
        responseType: 'text',
        responseEncoding: 'utf-8'
    })
        .then(response => {
            document.getElementById('license').textContent = response.data;
        })
        .catch(error => {
            console.error(error);
            document.getElementById('license').textContent = '无法加载声明文件';
        });
}

window.addEventListener('load', () => {
    getLicense();
});