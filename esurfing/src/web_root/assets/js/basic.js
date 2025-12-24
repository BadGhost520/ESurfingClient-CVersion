document.getElementById("short-switch").addEventListener("click", () => {
  document.getElementById("sidebar-navs").classList.toggle("navs-active");
});

document.getElementById("dashboard").addEventListener("click", () => {
  window.location.replace("dashboard.html");
});

document.getElementById("logs").addEventListener("click", () => {
  window.location.replace("logs.html");
});

document.getElementById("settings").addEventListener("click", () => {
  window.location.replace("settings.html");
});

document.getElementById("about").addEventListener("click", () => {
  window.location.replace("about.html");
});
