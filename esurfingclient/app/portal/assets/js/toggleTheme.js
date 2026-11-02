/** 本地存储读写 (隐私模式下静默失败), 后续脚本共用 */
const storage = {
    get(key, fallback = '') {
        try { return localStorage.getItem(key) ?? fallback; } catch (_) { return fallback; }
    },
    set(key, value) {
        try { localStorage.setItem(key, value); } catch (_) { /* 忽略 */ }
    }
};

(function () {
  "use strict";

  const DEFAULT_THEME = "light";
  const CHECKED_THEME = "synthwave";
  const STORAGE_KEY = "selectedTheme";

  const themeToggle = document.getElementById("themeToggle");
  const htmlElement = document.documentElement;

  if (!themeToggle) return;

  /** 应用主题到 <html> */
  function applyTheme(theme) {
    htmlElement.setAttribute("data-theme", theme === CHECKED_THEME ? CHECKED_THEME : DEFAULT_THEME);
  }

  function loadTheme() {
    const saved = storage.get(STORAGE_KEY, null);
    const theme = saved === CHECKED_THEME ? CHECKED_THEME : DEFAULT_THEME;
    themeToggle.checked = theme === CHECKED_THEME;
    applyTheme(theme);
  }

  themeToggle.addEventListener("change", (event) => {
    const theme = event.target.checked ? CHECKED_THEME : DEFAULT_THEME;
    applyTheme(theme);
    storage.set(STORAGE_KEY, theme);
  });

  loadTheme();
})();
