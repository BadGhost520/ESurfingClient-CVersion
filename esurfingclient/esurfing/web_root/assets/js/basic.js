document.getElementById("short-switch").addEventListener("click", () => {
  document.getElementById("sidebar-navs").classList.toggle("navs-active");
});

document.getElementById("dashboard").addEventListener("click", () => {
  window.location.replace("/dashboard");
});

document.getElementById("logs").addEventListener("click", () => {
  window.location.replace("/logs");
});

document.getElementById("settings").addEventListener("click", () => {
  window.location.replace("/settings");
});

document.getElementById("about").addEventListener("click", () => {
  window.location.replace("/about");
});

let intervalId;
const loadingModalText = document.getElementById("loading-modal-text");
const loadingModal = document.getElementById("loading-modal");
function waitSettingsSave() {
  loadingModal.classList.add("modalact");
  let countdown = 30;
  loadingModalText.textContent = `正在等待配置应用, 剩余时间: ${countdown} 秒`;
  countdown--;
  intervalId = setInterval(async () => {
    if (countdown < 0) {
      clearInterval(intervalId);
      loadingModalText.textContent = "配置应用超时"
      await sleep(1000);
      loadingModal.classList.remove("modalact");
      return;
    }
    loadingModalText.textContent = `正在等待配置应用, 剩余时间: ${countdown} 秒`;
    countdown--;
  }, 1000);
}

async function closeLoadingModal(status) {
  clearInterval(intervalId);
  if (status) {
    loadingModalText.textContent = "配置应用成功"
  } else {
    loadingModalText.textContent = "配置应用失败"
  }
  await sleep(1000);
  loadingModal.classList.remove("modalact");
}

function sleep(time) {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve();
    }, time);
  });
}