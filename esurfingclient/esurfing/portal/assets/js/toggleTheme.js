(function () {
  const DEFAULT_THEME = "light";
  const CHECKED_THEME = "synthwave";

  const themeToggle = document.getElementById("themeToggle");
  const htmlElement = document.documentElement;

  function loadTheme() {
    const savedTheme = localStorage.getItem("selectedTheme");
    if (savedTheme === CHECKED_THEME) {
      themeToggle.checked = true;
      htmlElement.setAttribute("data-theme", CHECKED_THEME);
    } else {
      themeToggle.checked = false;
      htmlElement.setAttribute("data-theme", DEFAULT_THEME);
    }
  }

  function saveTheme(isChecked) {
    const newTheme = isChecked ? CHECKED_THEME : DEFAULT_THEME;
    htmlElement.setAttribute("data-theme", newTheme);
    localStorage.setItem("selectedTheme", newTheme);
  }

  themeToggle.addEventListener("change", (e) => {
    saveTheme(e.target.checked);
  });

  loadTheme();
})();
