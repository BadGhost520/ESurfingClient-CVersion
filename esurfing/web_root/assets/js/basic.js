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
