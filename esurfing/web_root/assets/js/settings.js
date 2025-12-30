// 静态内容
const staticPwdInput = document.getElementById("static-pwd");
const staticUsrInput = document.getElementById("static-usr");
const staticChnPC = document.getElementById("static-chn-pc");
const staticChnPhone = document.getElementById("static-chn-phone");
const staticDebugOn = document.getElementById("static-debug-on");
const staticDebugOff = document.getElementById("static-debug-off");
const staticSmallOn = document.getElementById("static-small-on");
const staticSmallOff = document.getElementById("static-small-off");

// 模态框
const settingsModalBtn = document.getElementById("change-btn");
const settingsModal = document.getElementById("modal");
const modalContainer = document.getElementById("modal-container");
const cancelBtn = document.getElementById("cancel");
const usrInput = document.getElementById("usr");
const pwdInput = document.getElementById("pwd");
const chn = document.getElementById("chn");
const debug = document.getElementById("debug");
const smallDevice = document.getElementById("smallDevice");
const channelPCBtn = document.getElementById("channel-pc");
const channelPhoneBtn = document.getElementById("channel-phone");
const debugOnBtn = document.getElementById("debug-on");
const debugOffBtn = document.getElementById("debug-off");
const smallDeviceOnBtn = document.getElementById("small-device-on");
const smallDeviceOffBtn = document.getElementById("small-device-off");

channelPCBtn.addEventListener("click", () => {
  chn.value = "pc";
  channelPCBtn.classList.add("btnon");
  channelPhoneBtn.classList.remove("btnon");
});

channelPhoneBtn.addEventListener("click", () => {
  chn.value = "phone";
  channelPCBtn.classList.remove("btnon");
  channelPhoneBtn.classList.add("btnon");
});

debugOnBtn.addEventListener("click", () => {
  debug.value = "1";
  debugOnBtn.classList.add("btnon");
  debugOffBtn.classList.remove("btnon");
});

debugOffBtn.addEventListener("click", () => {
  debug.value = "0";
  debugOnBtn.classList.remove("btnon");
  debugOffBtn.classList.add("btnon");
});

smallDeviceOnBtn.addEventListener("click", () => {
  smallDevice.value = "1";
  smallDeviceOnBtn.classList.add("btnon");
  smallDeviceOffBtn.classList.remove("btnon");
});

smallDeviceOffBtn.addEventListener("click", () => {
  smallDevice.value = "0";
  smallDeviceOnBtn.classList.remove("btnon");
  smallDeviceOffBtn.classList.add("btnon");
});

function resetModal() {
  usrInput.value = "";
  pwdInput.value = "";
  chn.value = "phone";
  debug.value = "0";
  smallDevice.value = "0";
  channelPCBtn.classList.remove("btnon");
  channelPhoneBtn.classList.add("btnon");
  debugOnBtn.classList.remove("btnon");
  debugOffBtn.classList.add("btnon");
  smallDeviceOnBtn.classList.remove("btnon");
  smallDeviceOffBtn.classList.add("btnon");
}

settingsModalBtn.addEventListener("click", () => {
  if (!settingsModal.classList.contains("modalact"))
    settingsModal.classList.add("modalact");
});

document.addEventListener("click", (e) => {
  if (
      (!modalContainer.contains(e.target) &&
          !settingsModalBtn.contains(e.target) &&
          settingsModal.classList.contains("modalact")) ||
      cancelBtn.contains(e.target)
  ) {
    settingsModal.classList.remove("modalact");
    resetModal();
  }
});

// 远程
async function getSettings() {
  await axios({
    method: "get",
    url: "/api/getSettings",
    timeout: 5000,
    responseType: "json",
    responseEncoding: "utf-8",
  })
      .then((response) => {
        const data = response.data;
        staticUsrInput.value = data.username;
        staticPwdInput.value = data.password;

        if (data.channel === "pc") {
          staticChnPhone.classList.remove("btnon");
          staticChnPC.classList.add("btnon");
        } else {
          staticChnPC.classList.remove("btnon");
          staticChnPhone.classList.add("btnon");
        }

        if (data.debug) {
          staticDebugOff.classList.remove("btnon");
          staticDebugOn.classList.add("btnon");
        } else {
          staticDebugOn.classList.remove("btnon");
          staticDebugOff.classList.add("btnon");
        }

        if (data.smallDevice) {
          staticSmallOff.classList.remove("btnon");
          staticSmallOn.classList.add("btnon");
        } else {
          staticSmallOn.classList.remove("btnon");
          staticSmallOff.classList.add("btnon");
        }
      })
      .catch((error) => {
        console.error(error);
      });
}

// 设置表单提交
const settingsSubmitBtn = document.getElementById("submit");

async function updateSettings() {
  const debugNum = debug.value === "1" ? 1 : 0;
  const smallDeviceNum = smallDevice.value === "1" ? 1 : 0;
  await axios({
    method: "post",
    url: "/api/updateSettings",
    timeout: 5000,
    headers: {
      "Content-Type": "application/json",
    },
    responseType: "json",
    responseEncoding: "utf-8",
    data: {
      username: usrInput.value,
      password: pwdInput.value,
      channel: chn.value,
      debug: debugNum,
      smallDevice: smallDeviceNum,
    },
  })
      .then((response) => {
        if (response.status === 204) {
          alert("设置已保存");
        } else {
          alert("设置保存失败");
        }
      })
      .catch((error) => {
        console.error(error);
      });
}

settingsSubmitBtn.addEventListener("click", () => {
  if (usrInput.value === "" || pwdInput.value === "") {
    alert("用户名或密码不能为空");
    return;
  }
  updateSettings();
  resetModal();
  settingsModal.classList.remove("modalact");
  getSettings();
});

window.addEventListener("load", () => {
  getSettings();
});
