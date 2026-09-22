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

  /** 读取本地保存的主题 */
  function readSavedTheme() {
    try {
      return localStorage.getItem(STORAGE_KEY);
    } catch (error) {
      // 隐私模式 / 禁用存储时忽略
      return null;
    }
  }

  /** 保存主题 */
  function saveTheme(theme) {
    try {
      localStorage.setItem(STORAGE_KEY, theme);
    } catch (error) {
      // 忽略保存失败
    }
  }

  function loadTheme() {
    const saved = readSavedTheme();
    const theme = saved === CHECKED_THEME ? CHECKED_THEME : DEFAULT_THEME;
    themeToggle.checked = theme === CHECKED_THEME;
    applyTheme(theme);
  }

  themeToggle.addEventListener("change", (event) => {
    const theme = event.target.checked ? CHECKED_THEME : DEFAULT_THEME;
    applyTheme(theme);
    saveTheme(theme);
  });

  loadTheme();
})();
