/* llm2lvgl Web UI app logic */
(function () {
  "use strict";

  const $ = (s, el) => (el || document).querySelector(s);
  const $$ = (s, el) => [...(el || document).querySelectorAll(s)];

  const STEPS = ["init", "generate", "lint", "build", "validate", "refine", "export"];

  let currentTaskId = null;
  let evtSource = null;

  // -- Init -----------------------------------------------------------------

  async function init() {
    await loadProfiles();
    await loadSettings();
    await loadHistory();
    setupTabs();
    setupDropZones();
    $("#start-btn").addEventListener("click", onStart);
    $("#stop-btn").addEventListener("click", onStop);
    $("#save-settings-btn").addEventListener("click", saveSettings);
  }

  async function loadProfiles() {
    const res = await fetch("/api/profiles");
    const profiles = await res.json();
    const sel = $("#profile-select");
    sel.innerHTML = "";
    for (const p of profiles) {
      const opt = document.createElement("option");
      opt.value = p.file;
      opt.textContent = `${p.name} (${p.screen.width}x${p.screen.height})`;
      sel.appendChild(opt);
    }
  }

  // -- Settings -------------------------------------------------------------

  async function loadSettings() {
    try {
      const res = await fetch("/api/settings");
      const data = await res.json();
      $("#setting-api-key").value = data.api_key || "";
      const modelSel = $("#setting-model");
      const savedModel = data.model || "";
      // If saved model isn't in the dropdown, add it dynamically
      if (savedModel && !modelSel.querySelector(`option[value="${savedModel}"]`)) {
        const opt = document.createElement("option");
        opt.value = savedModel;
        opt.textContent = savedModel;
        modelSel.prepend(opt);
      }
      modelSel.value = savedModel;
      $("#setting-base-url").value = data.base_url || "";
    } catch (e) {
      // ignore – settings are optional
    }
  }

  async function saveSettings() {
    const msg = $("#settings-msg");
    const body = {
      api_key: $("#setting-api-key").value,
      model: $("#setting-model").value,
      base_url: $("#setting-base-url").value,
    };
    try {
      const res = await fetch("/api/settings", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
      });
      if (res.ok) {
        msg.textContent = "Saved";
        msg.style.color = "var(--success)";
      } else {
        const err = await res.json();
        msg.textContent = err.error || "Failed";
        msg.style.color = "var(--error)";
      }
    } catch (e) {
      msg.textContent = "Error: " + e.message;
      msg.style.color = "var(--error)";
    }
    setTimeout(() => { msg.textContent = ""; }, 3000);
  }

  // -- Tabs -----------------------------------------------------------------

  function setupTabs() {
    for (const btn of $$(".tab-btn")) {
      btn.addEventListener("click", () => {
        $$(".tab-btn").forEach((b) => b.classList.remove("active"));
        $$(".tab-content").forEach((c) => c.classList.remove("active"));
        btn.classList.add("active");
        $(`#tab-${btn.dataset.tab}`).classList.add("active");
      });
    }
  }

  // -- Drop zones -----------------------------------------------------------

  // Collected files (DataTransfer doesn't persist across events reliably)
  let htmlFiles = [];
  let imageFiles = [];

  function setupDropZones() {
    setupDropZone("html-drop-zone", "html-file", "html-file-list", (files) => { htmlFiles = files; });
    setupDropZone("image-drop-zone", "image-file", "image-file-list", (files) => { imageFiles = files; });
  }

  function setupDropZone(zoneId, inputId, listId, onUpdate) {
    const zone = document.getElementById(zoneId);
    const input = document.getElementById(inputId);
    const listEl = document.getElementById(listId);
    if (!zone || !input) return;

    zone.addEventListener("click", () => input.click());
    zone.addEventListener("dragover", (e) => { e.preventDefault(); zone.classList.add("dragover"); });
    zone.addEventListener("dragleave", () => zone.classList.remove("dragover"));
    zone.addEventListener("drop", (e) => {
      e.preventDefault();
      zone.classList.remove("dragover");
      if (e.dataTransfer.files.length) {
        const files = Array.from(e.dataTransfer.files);
        onUpdate(files);
        renderFileList(listEl, files);
      }
    });
    input.addEventListener("change", () => {
      if (input.files.length) {
        const files = Array.from(input.files);
        onUpdate(files);
        renderFileList(listEl, files);
      }
    });
  }

  function renderFileList(listEl, files) {
    listEl.innerHTML = "";
    for (const f of files) {
      const div = document.createElement("div");
      div.className = "file-item";
      div.textContent = f.name + " (" + (f.size / 1024).toFixed(1) + " KB)";
      listEl.appendChild(div);
    }
  }

  // -- Start task -----------------------------------------------------------

  async function onStart() {
    const btn = $("#start-btn");
    btn.disabled = true;
    $("#stop-btn").style.display = "inline-block";

    const activeTab = $(".tab-btn.active").dataset.tab;
    const profile = $("#profile-select").value;
    const name = $("#task-name").value.trim();
    const fd = new FormData();
    fd.append("profile", profile);
    if (name) fd.append("name", name);

    if (activeTab === "html") {
      fd.append("source_type", "html");
      if (!htmlFiles.length) {
        const f = $("#html-file").files[0];
        if (!f) { alert("Please select an HTML file"); btn.disabled = false; return; }
        htmlFiles = [f];
      }
      // First .html/.htm file is the main entry
      const mainHtml = htmlFiles.find(f => /\.html?$/i.test(f.name));
      if (!mainHtml) { alert("No HTML file found in selection"); btn.disabled = false; return; }
      fd.append("file", mainHtml);
      // Additional assets
      for (const f of htmlFiles) {
        if (f !== mainHtml) fd.append("assets", f);
      }
    } else if (activeTab === "image") {
      fd.append("source_type", "image");
      if (!imageFiles.length) {
        const f = $("#image-file").files[0];
        if (!f) { alert("Please select an image"); btn.disabled = false; return; }
        imageFiles = [f];
      }
      // First image is the main screenshot
      fd.append("file", imageFiles[0]);
      // Additional images as assets
      for (let i = 1; i < imageFiles.length; i++) {
        fd.append("assets", imageFiles[i]);
      }
    } else {
      fd.append("source_type", "url");
      const url = $("#url-input").value.trim();
      if (!url) { alert("Please enter a URL"); btn.disabled = false; return; }
      fd.append("url", url);
    }

    try {
      // Create task
      let res = await fetch("/api/tasks", { method: "POST", body: fd });
      let data = await res.json();
      if (!res.ok) { alert(data.error || "Failed to create task"); btn.disabled = false; return; }
      currentTaskId = data.task_id;

      // Reset UI
      resetProgress();
      $(".log-box").classList.add("visible");
      $(".results").classList.remove("visible");

      // Run pipeline
      res = await fetch(`/api/tasks/${currentTaskId}/run`, { method: "POST" });
      data = await res.json();
      if (!res.ok) { alert(data.error || "Failed to start"); btn.disabled = false; return; }

      // Connect SSE
      connectSSE(currentTaskId);
    } catch (e) {
      alert("Error: " + e.message);
      btn.disabled = false;
      $("#stop-btn").style.display = "none";
    }
  }

  // -- Stop task -----------------------------------------------------------

  async function onStop() {
    if (!currentTaskId) return;
    try {
      await fetch(`/api/tasks/${currentTaskId}/stop`, { method: "POST" });
    } catch (e) {
      // ignore – SSE done event will handle UI reset
    }
  }

  // -- SSE ------------------------------------------------------------------

  function connectSSE(taskId) {
    if (evtSource) evtSource.close();
    const logBox = $(".log-box");
    evtSource = new EventSource(`/api/tasks/${taskId}/stream`);

    evtSource.addEventListener("log", (e) => {
      const line = JSON.parse(e.data);
      logBox.textContent += line + "\n";
      logBox.scrollTop = logBox.scrollHeight;
    });

    evtSource.addEventListener("step", (e) => {
      const step = JSON.parse(e.data);
      updateSteps(step);
    });

    evtSource.addEventListener("done", (e) => {
      const info = JSON.parse(e.data);
      evtSource.close();
      evtSource = null;
      $("#start-btn").disabled = false;
      $("#stop-btn").style.display = "none";
      if (info.status === "done") {
        markAllStepsDone();
        loadResults(taskId);
      } else {
        markStepFailed();
      }
      loadHistory();
    });

    evtSource.onerror = () => {
      evtSource.close();
      evtSource = null;
      $("#start-btn").disabled = false;
      $("#stop-btn").style.display = "none";
    };
  }

  // -- Steps UI -------------------------------------------------------------

  function resetProgress() {
    for (const el of $$(".step")) {
      el.classList.remove("active", "done", "failed");
    }
  }

  function updateSteps(current) {
    const idx = STEPS.indexOf(current);
    for (const el of $$(".step")) {
      const si = STEPS.indexOf(el.dataset.step);
      el.classList.remove("active", "done", "failed");
      if (si < idx) el.classList.add("done");
      else if (si === idx) el.classList.add("active");
    }
  }

  function markAllStepsDone() {
    for (const el of $$(".step")) el.classList.add("done");
  }

  function markStepFailed() {
    const active = $(".step.active");
    if (active) { active.classList.remove("active"); active.classList.add("failed"); }
  }

  // -- Results --------------------------------------------------------------

  async function loadResults(taskId) {
    const res = await fetch(`/api/tasks/${taskId}`);
    const data = await res.json();
    const results = $(".results");
    results.classList.add("visible");

    // Images
    const setImg = (sel, name) => {
      const img = $(sel);
      if (data.artifacts[name]) {
        img.src = `/api/tasks/${taskId}/artifacts/${name}`;
        img.parentElement.style.display = "";
      } else {
        img.parentElement.style.display = "none";
      }
    };
    setImg("#img-ref", "current.png");
    setImg("#img-actual", "full.png");
    setImg("#img-diff", "diff.png");

    // Report
    if (data.artifacts["report.json"]) {
      const rr = await fetch(`/api/tasks/${taskId}/artifacts/report.json`);
      const report = await rr.json();
      const summary = $("#report-summary");
      const passed = report.pass === true || report.passed === true;
      summary.innerHTML = `
        <span class="${passed ? "pass" : "fail"}">${passed ? "PASS" : "FAIL"}</span>
        &nbsp; diff_ratio: ${(report.diff_ratio ?? 0).toFixed(4)}
        &nbsp; mean_abs_diff: ${(report.mean_abs_diff ?? 0).toFixed(2)}
      `;
    }

    // Download button
    $("#download-btn").onclick = () => {
      window.location = `/api/tasks/${taskId}/export`;
    };

    // Code preview
    const codeRes = await fetch(`/api/tasks/${taskId}/code`);
    const codeData = await codeRes.json();
    const codeBlock = $("#code-block");
    codeBlock.textContent = Object.entries(codeData.files || {})
      .map(([name, content]) => `// --- ${name} ---\n${content}`)
      .join("\n\n");
  }

  // -- History --------------------------------------------------------------

  async function loadHistory() {
    const res = await fetch("/api/tasks");
    const tasks = await res.json();
    const list = $("#task-list");
    list.innerHTML = "";
    for (const t of tasks.reverse()) {
      const li = document.createElement("li");
      li.textContent = t.page_name || t.task_id;
      const sp = document.createElement("span");
      sp.className = "status";
      sp.textContent = t.status;
      li.appendChild(sp);
      li.addEventListener("click", () => {
        currentTaskId = t.task_id;
        loadResults(t.task_id);
      });
      list.appendChild(li);
    }
  }

  // -- Code toggle ----------------------------------------------------------

  document.addEventListener("DOMContentLoaded", () => {
    const toggle = $(".code-toggle");
    if (toggle) {
      toggle.addEventListener("click", () => {
        $("#code-block").classList.toggle("visible");
      });
    }
    init();
  });
})();
