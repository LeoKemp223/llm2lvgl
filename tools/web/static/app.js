/* llm2lvgl Codegen Console */
(function () {
  "use strict";

  const $ = (s, el) => (el || document).querySelector(s);
  const $$ = (s, el) => [...(el || document).querySelectorAll(s)];

  const STEPS = ["init", "analyze", "generate", "lint", "build", "validate", "refine", "export"];
  const STEP_LABELS = {
    init: "初始化",
    analyze: "分析",
    generate: "生成",
    lint: "检查",
    build: "构建",
    validate: "验证",
    refine: "精调",
    export: "导出",
  };

  let currentTaskId = null;
  let evtSource = null;
  let recoveryTimer = null;
  let profilesData = [];
  let htmlFiles = [];
  let imageFiles = [];
  let pendingAnalysis = null;
  let pendingTargetContext = null;

  async function init() {
    bindStaticEvents();
    setDownloadEnabled(false);

    try {
      await Promise.all([loadProfiles(), loadSettings(), loadHistory()]);
      setGlobalStatus("就绪");
    } catch (e) {
      showNotice("初始化失败: " + e.message, "error");
      showToast("初始化失败: " + e.message, "error");
      setGlobalStatus("异常", "failed");
    }
  }

  function bindStaticEvents() {
    setupTabs();
    setupDropZones();

    $("#start-btn").addEventListener("click", onStart);
    $("#stop-btn").addEventListener("click", onStop);
    $("#analysis-confirm-btn").addEventListener("click", onConfirmAnalysis);
    $("#analysis-edit-btn").addEventListener("click", applyAnalysisMode);
    $("#analysis-cancel-btn").addEventListener("click", cancelAnalysis);
    $("#target-confirm-btn").addEventListener("click", onConfirmTarget);
    $("#target-use-current-btn").addEventListener("click", useCurrentTargetConfig);
    $("#target-cancel-btn").addEventListener("click", cancelTargetConfirm);
    $("#save-settings-btn").addEventListener("click", saveSettings);
    ["custom-profile-name", "custom-width", "custom-height"].forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.addEventListener("input", updateProfileSummary);
    });

    const clearLogBtn = $("#clear-log-btn");
    if (clearLogBtn) {
      clearLogBtn.addEventListener("click", () => {
        $(".log-box").textContent = "";
        $(".log-box").classList.remove("visible");
      });
    }

    const toggle = $(".code-toggle");
    if (toggle) {
      toggle.addEventListener("click", () => {
        const codeBlock = $("#code-block");
        codeBlock.classList.toggle("visible");
        toggle.textContent = codeBlock.classList.contains("visible") ? "收起生成代码" : "查看生成代码";
      });
    }

    const lightbox = $("#lightbox");
    const lightboxImg = $("#lightbox-img");
    document.addEventListener("click", (e) => {
      if (e.target.matches(".image-compare img")) {
        lightboxImg.src = e.target.src;
        lightbox.classList.add("open");
      } else if (e.target.matches(".lightbox") || e.target.matches(".lightbox img")) {
        lightbox.classList.remove("open");
      }
    });
    document.addEventListener("keydown", (e) => {
      if (e.key === "Escape") lightbox.classList.remove("open");
      if ((e.key === "Enter" || e.key === " ") && e.target.matches(".image-compare img")) {
        e.preventDefault();
        lightboxImg.src = e.target.src;
        lightbox.classList.add("open");
      }
    });
  }

  async function loadProfiles() {
    const res = await fetch("/api/profiles");
    if (!res.ok) throw new Error("无法加载目标平台");
    profilesData = await res.json();

    const sel = $("#profile-select");
    sel.innerHTML = "";
    for (const p of profilesData) {
      const opt = document.createElement("option");
      opt.value = p.file;
      opt.textContent = `${p.name} (${p.screen.width}x${p.screen.height})`;
      sel.appendChild(opt);
    }

    const customOpt = document.createElement("option");
    customOpt.value = "__custom__";
    customOpt.textContent = "自定义平台...";
    sel.appendChild(customOpt);
    sel.addEventListener("change", onProfileChange);
    onProfileChange();
  }

  function onProfileChange() {
    const sel = $("#profile-select");
    const custom = $("#custom-profile");
    if (sel.value === "__custom__") {
      custom.style.display = "block";
      updateProfileSummary();
      return;
    }

    custom.style.display = "none";
    const p = profilesData.find((x) => x.file === sel.value);
    if (p) {
      $("#custom-width").value = p.screen.width || "";
      $("#custom-height").value = p.screen.height || "";
      $("#custom-profile-name").value = p.name || "";
    }
    updateProfileSummary();
  }

  async function loadSettings() {
    try {
      const res = await fetch("/api/settings");
      if (!res.ok) return;
      const data = await res.json();
      $("#setting-api-key").value = data.api_key || "";
      $("#setting-model").value = data.model || "";
      $("#setting-base-url").value = data.base_url || "";
    } catch (e) {
      // Settings are optional during first run.
    }
  }

  async function saveSettings() {
    const msg = $("#settings-msg");
    msg.textContent = "保存中...";
    msg.style.color = "var(--muted)";

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
        msg.textContent = "已保存";
        msg.style.color = "var(--success)";
        showToast("模型设置已保存", "success");
      } else {
        const err = await safeJson(res);
        msg.textContent = err.error || "保存失败";
        msg.style.color = "var(--error)";
        showToast(msg.textContent, "error");
      }
    } catch (e) {
      msg.textContent = "错误: " + e.message;
      msg.style.color = "var(--error)";
      showToast(msg.textContent, "error");
    }

    setTimeout(() => {
      msg.textContent = "";
    }, 3000);
  }

  function setupTabs() {
    for (const btn of $$(".tab-btn")) {
      btn.addEventListener("click", () => {
        $$(".tab-btn").forEach((b) => {
          b.classList.remove("active");
          b.setAttribute("aria-selected", "false");
        });
        $$(".tab-content").forEach((c) => c.classList.remove("active"));
        btn.classList.add("active");
        btn.setAttribute("aria-selected", "true");
        $(`#tab-${btn.dataset.tab}`).classList.add("active");
        clearNotice();
      });
    }
  }

  function setupDropZones() {
    setupDropZone("html-drop-zone", "html-file", "html-file-list", (files) => {
      htmlFiles = files;
      inferTargetFromFiles("html", files);
    }, htmlFileRole);
    setupDropZone("image-drop-zone", "image-file", "image-file-list", (files) => {
      imageFiles = files;
      inferTargetFromFiles("image", files);
    }, imageFileRole);
  }

  function setupDropZone(zoneId, inputId, listId, onUpdate, roleResolver) {
    const zone = document.getElementById(zoneId);
    const input = document.getElementById(inputId);
    const listEl = document.getElementById(listId);
    if (!zone || !input || !listEl) return;

    zone.addEventListener("click", () => input.click());
    zone.addEventListener("dragover", (e) => {
      e.preventDefault();
      zone.classList.add("dragover");
    });
    zone.addEventListener("dragleave", () => zone.classList.remove("dragover"));
    zone.addEventListener("drop", (e) => {
      e.preventDefault();
      zone.classList.remove("dragover");
      if (!e.dataTransfer.files.length) return;
      const files = Array.from(e.dataTransfer.files);
      onUpdate(files);
      renderFileList(listEl, files, roleResolver);
      clearNotice();
    });
    input.addEventListener("change", () => {
      if (!input.files.length) return;
      const files = Array.from(input.files);
      onUpdate(files);
      renderFileList(listEl, files, roleResolver);
      clearNotice();
    });
  }

  function renderFileList(listEl, files, roleResolver) {
    for (const old of listEl.querySelectorAll("img[data-blob]")) {
      URL.revokeObjectURL(old.src);
    }
    listEl.innerHTML = "";

    files.forEach((f, index) => {
      const div = document.createElement("div");
      div.className = "file-item";
      const sizeStr = formatSize(f.size);
      const isImage = /^image\//.test(f.type) || /\.(png|jpe?g|gif|webp|bmp|svg)$/i.test(f.name);

      if (isImage) {
        const url = URL.createObjectURL(f);
        const img = document.createElement("img");
        img.src = url;
        img.alt = f.name;
        img.className = "file-thumb";
        img.dataset.blob = "1";
        img.title = `${f.name} (${sizeStr})，点击放大`;
        img.addEventListener("click", (e) => {
          e.stopPropagation();
          $("#lightbox-img").src = url;
          $("#lightbox").classList.add("open");
        });
        div.appendChild(img);
      } else {
        const icon = document.createElement("span");
        icon.className = "file-icon";
        icon.textContent = fileKind(f.name);
        div.appendChild(icon);
      }

      const label = document.createElement("span");
      label.className = "file-label";
      label.textContent = `${f.name} (${sizeStr})`;
      div.appendChild(label);

      const role = document.createElement("span");
      role.className = "file-role";
      role.textContent = roleResolver ? roleResolver(f, index, files) : (index === 0 ? "主文件" : "资源");
      div.appendChild(role);

      listEl.appendChild(div);
    });
  }

  async function onStart() {
    clearNotice();
    setDownloadEnabled(false);
    stopRecoveryPolling();

    const validation = await validateAndBuildForm();
    if (!validation.ok) {
      showNotice(validation.error, "error");
      showToast(validation.error, "error");
      focusInvalid(validation.targetId);
      setRunningState(false);
      return;
    }

    pendingTargetContext = validation;
    showTargetModal(validation.target);
  }

  async function createAndAnalyzeTask(validation) {
    if (!validation || !validation.ok) return;

    setRunningState(true);
    resetProgress();
    resetResults();
    $(".log-box").textContent = "";
    $(".log-box").classList.add("visible");
    appendLog("正在准备任务参数...");

    try {
      let res = await fetch("/api/tasks", { method: "POST", body: validation.formData });
      let data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || data.detail || "创建任务失败，请检查输入源、平台配置和 API 设置后重试");

      currentTaskId = data.task_id;
      setTaskContext(currentTaskId);
      appendLog(`任务已创建：${currentTaskId}`);

      updateSteps("analyze");
      setGlobalStatus("分析中", "running");
      appendLog("正在分析页面结构和验证方式...");
      res = await fetch(`/api/tasks/${currentTaskId}/analyze`, { method: "POST" });
      data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || data.detail || "页面分析失败，请检查模型设置后重试");
      if (data.log) data.log.split(/\r?\n/).filter(Boolean).forEach(appendLog);

      pendingAnalysis = data.analysis || {};
      showAnalysisModal(pendingAnalysis);
      setAwaitingConfirmationState();
      showNotice("请确认页面分析结果，确认后继续生成。", "success");
    } catch (e) {
      appendLog("ERROR: " + e.message);
      markStepFailed();
      setRunningState(false);
      setGlobalStatus("失败", "failed");
      showNotice(e.message, "error");
      showToast(e.message, "error");
      loadHistory();
    }
  }

  async function onConfirmAnalysis() {
    if (!currentTaskId || !pendingAnalysis) return;
    applyAnalysisMode();
    closeAnalysisModal();
    appendLog("用户已确认页面分析结果。");
    setRunningState(true);
    try {
      let res = await fetch(`/api/tasks/${currentTaskId}/analysis/confirm`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ analysis: pendingAnalysis }),
      });
      let data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || data.detail || "确认分析失败");

      res = await fetch(`/api/tasks/${currentTaskId}/run`, { method: "POST" });
      data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || "启动失败，请稍后重试");

      showNotice(`任务 ${currentTaskId} 已启动`, "success");
      connectSSE(currentTaskId);
    } catch (e) {
      appendLog("ERROR: " + e.message);
      markStepFailed();
      setRunningState(false);
      setGlobalStatus("失败", "failed");
      showNotice(e.message, "error");
      showToast(e.message, "error");
      loadHistory();
    }
  }

  function applyAnalysisMode() {
    if (!pendingAnalysis) return;
    pendingAnalysis.validation_mode = $("#analysis-validation-mode").value;
    pendingAnalysis.confirmed = true;
    renderAnalysisSummary(pendingAnalysis);
  }

  function cancelAnalysis() {
    closeAnalysisModal();
    appendLog("--- 等待用户确认页面分析结果 ---");
    setRunningState(false);
    setGlobalStatus("待确认");
    showNotice("任务已创建，尚未开始生成。", "success");
    loadHistory();
  }

  async function validateAndBuildForm() {
    clearFieldErrors();
    const activeTab = $(".tab-btn.active").dataset.tab;
    const profile = $("#profile-select").value;
    const name = $("#task-name").value.trim();
    const fd = new FormData();

    if (!profile) {
      return invalid("profile-select", "请选择目标平台");
    }

    if (profile === "__custom__") {
      const cw = parseInt($("#custom-width").value, 10);
      const ch = parseInt($("#custom-height").value, 10);
      const cname = $("#custom-profile-name").value.trim();
      if (!cw || !ch || cw < 100 || ch < 100) {
        return invalid(!cw || cw < 100 ? "custom-width" : "custom-height", "请输入有效的自定义分辨率，宽高均不小于 100");
      }
      fd.append("profile", "__custom__");
      fd.append("custom_name", cname || `${cw}x${ch}`);
      fd.append("custom_width", cw);
      fd.append("custom_height", ch);
    } else {
      fd.append("profile", profile);
    }

    if (name) fd.append("name", name);

    if (activeTab === "html") {
      const files = htmlFiles.length ? htmlFiles : Array.from($("#html-file").files || []);
      const mainHtml = files.find((f) => /\.html?$/i.test(f.name));
      if (!mainHtml) return invalid("html-drop-zone", "请上传 HTML 文件，资源文件可作为附加文件一起上传");
      fd.append("source_type", "html");
      fd.append("file", mainHtml);
      for (const f of files) {
        if (f !== mainHtml) fd.append("assets", f);
      }
    } else if (activeTab === "image") {
      const files = imageFiles.length ? imageFiles : Array.from($("#image-file").files || []);
      if (!files.length) return invalid("image-drop-zone", "请上传设计截图");
      fd.append("source_type", "image");
      fd.append("file", files[0]);
      for (let i = 1; i < files.length; i++) {
        fd.append("assets", files[i]);
      }
    } else {
      const url = $("#url-input").value.trim();
      if (!url) return invalid("url-input", "请输入页面网址");
      try {
        new URL(url);
      } catch (e) {
        return invalid("url-input", "页面网址格式不正确，请输入完整的 http 或 https 地址");
      }
      fd.append("source_type", "url");
      fd.append("url", url);
    }

    const target = await inferTargetForCurrentInput(activeTab);
    return { ok: true, formData: fd, target };
  }

  async function onConfirmTarget() {
    if (!pendingTargetContext) return;
    const width = parseInt($("#target-width").value, 10);
    const height = parseInt($("#target-height").value, 10);
    const name = $("#target-profile-name").value.trim() || `${width}x${height}`;
    if (!width || !height || width < 100 || height < 100) {
      showToast("请输入有效的目标分辨率，宽高均不小于 100", "error");
      return;
    }
    applyCustomTarget(name, width, height);
    const fd = pendingTargetContext.formData;
    fd.set("profile", "__custom__");
    fd.set("custom_name", name);
    fd.set("custom_width", String(width));
    fd.set("custom_height", String(height));
    closeTargetModal();
    const validation = pendingTargetContext;
    pendingTargetContext = null;
    await createAndAnalyzeTask(validation);
  }

  function useCurrentTargetConfig() {
    const current = currentFormTarget();
    $("#target-profile-name").value = current.name;
    $("#target-width").value = current.width;
    $("#target-height").value = current.height;
    $("#target-infer-note").textContent = "已载入当前表单中的目标平台配置，请确认后继续。";
  }

  function cancelTargetConfirm() {
    closeTargetModal();
    pendingTargetContext = null;
    setRunningState(false);
    setGlobalStatus("就绪");
  }

  function invalid(targetId, error) {
    return { ok: false, error, targetId };
  }

  async function inferTargetFromFiles(sourceType, files) {
    try {
      const target = await inferTargetFromInput(sourceType, files);
      if (!target) return;
      applyCustomTarget(target.name, target.width, target.height);
      showNotice(`已根据上传内容推断目标分辨率：${target.width}x${target.height}，开始前仍可确认修改。`, "success");
    } catch (e) {
      showToast("分辨率推断失败: " + e.message, "error");
    }
  }

  async function inferTargetForCurrentInput(activeTab) {
    if (activeTab === "html") {
      const files = htmlFiles.length ? htmlFiles : Array.from($("#html-file").files || []);
      return await inferTargetFromInput("html", files);
    }
    if (activeTab === "image") {
      const files = imageFiles.length ? imageFiles : Array.from($("#image-file").files || []);
      return await inferTargetFromInput("image", files);
    }
    const current = currentFormTarget();
    return {
      ...current,
      source: "当前表单",
      note: "远程 URL 无法在创建前可靠读取实际渲染尺寸，请确认目标平台分辨率。",
    };
  }

  async function inferTargetFromInput(sourceType, files) {
    if (sourceType === "image" && files && files[0]) {
      const size = await imageSize(files[0]);
      return {
        name: `上传图片 ${size.width}x${size.height}`,
        width: size.width,
        height: size.height,
        source: "上传图片",
        note: "已读取主图原始像素尺寸。",
      };
    }

    if (sourceType === "html" && files && files.length) {
      const mainHtml = files.find((f) => /\.html?$/i.test(f.name));
      if (mainHtml) {
        const text = await mainHtml.text();
        const parsed = parseHtmlViewportSize(text);
        if (parsed) {
          return {
            name: `HTML 页面 ${parsed.width}x${parsed.height}`,
            width: parsed.width,
            height: parsed.height,
            source: "HTML 文件",
            note: parsed.note,
          };
        }
      }
    }

    const current = currentFormTarget();
    return {
      ...current,
      source: "当前表单",
      note: "未从上传内容中找到明确尺寸，已使用当前表单配置。",
    };
  }

  function imageSize(file) {
    return new Promise((resolve, reject) => {
      const url = URL.createObjectURL(file);
      const img = new Image();
      img.onload = () => {
        const width = img.naturalWidth;
        const height = img.naturalHeight;
        URL.revokeObjectURL(url);
        if (width && height) resolve({ width, height });
        else reject(new Error("图片尺寸无效"));
      };
      img.onerror = () => {
        URL.revokeObjectURL(url);
        reject(new Error("无法读取图片尺寸"));
      };
      img.src = url;
    });
  }

  function parseHtmlViewportSize(html) {
    const candidates = [
      /<canvas[^>]*\bwidth=["']?(\d{2,5})["']?[^>]*\bheight=["']?(\d{2,5})["']?/i,
      /<svg[^>]*\bwidth=["']?(\d{2,5})["']?[^>]*\bheight=["']?(\d{2,5})["']?/i,
      /<(?:html|body|main|section|div)[^>]*(?:id|class)=["'][^"']*(?:app|root|screen|page|viewport|container)[^"']*["'][^>]*style=["'][^"']*(?:width|min-width)\s*:\s*(\d{2,5})px[^"']*(?:height|min-height)\s*:\s*(\d{2,5})px/i,
      /<(?:html|body|main|section|div)[^>]*style=["'][^"']*(?:width|min-width)\s*:\s*(\d{2,5})px[^"']*(?:height|min-height)\s*:\s*(\d{2,5})px/i,
      /(?:html|body|#app|#root|\.screen|\.page|\.viewport|\.container)\s*\{[^}]*(?:width|min-width)\s*:\s*(\d{2,5})px[^}]*(?:height|min-height)\s*:\s*(\d{2,5})px/i,
    ];
    for (let i = 0; i < candidates.length; i++) {
      const m = html.match(candidates[i]);
      if (!m) continue;
      let width = parseInt(m[1], 10);
      let height = parseInt(m[2], 10);
      if (width >= 100 && height >= 100) {
        return { width, height, note: "已从 HTML/CSS 中识别页面宽高。" };
      }
    }
    const viewport = html.match(/<meta[^>]+name=["']viewport["'][^>]+content=["']([^"']+)["']/i);
    if (viewport) {
      const widthMatch = viewport[1].match(/width\s*=\s*(\d{2,5})/i);
      const heightMatch = viewport[1].match(/height\s*=\s*(\d{2,5})/i);
      if (widthMatch && heightMatch) {
        const width = parseInt(widthMatch[1], 10);
        const height = parseInt(heightMatch[1], 10);
        if (width >= 100 && height >= 100) {
          return { width, height, note: "已从 viewport meta 中识别页面宽高。" };
        }
      }
    }
    return null;
  }

  function currentFormTarget() {
    const sel = $("#profile-select");
    if (sel.value === "__custom__") {
      return {
        name: $("#custom-profile-name").value.trim() || "自定义平台",
        width: parseInt($("#custom-width").value, 10) || 480,
        height: parseInt($("#custom-height").value, 10) || 480,
      };
    }
    const p = profilesData.find((x) => x.file === sel.value);
    return {
      name: p ? p.name : "目标平台",
      width: p && p.screen ? p.screen.width : 480,
      height: p && p.screen ? p.screen.height : 480,
    };
  }

  function applyCustomTarget(name, width, height) {
    $("#profile-select").value = "__custom__";
    $("#custom-profile").style.display = "block";
    $("#custom-profile-name").value = name || `${width}x${height}`;
    $("#custom-width").value = width;
    $("#custom-height").value = height;
    updateProfileSummary();
  }

  function showTargetModal(target) {
    const current = target || currentFormTarget();
    $("#target-profile-name").value = current.name || `${current.width}x${current.height}`;
    $("#target-width").value = current.width || 480;
    $("#target-height").value = current.height || 480;
    $("#target-infer-note").textContent = current.note || "请确认目标平台分辨率，确认后再创建任务。";
    const modal = $("#target-modal");
    modal.classList.add("open");
    modal.setAttribute("aria-hidden", "false");
    $("#target-confirm-btn").focus({ preventScroll: true });
  }

  function closeTargetModal() {
    const modal = $("#target-modal");
    modal.classList.remove("open");
    modal.setAttribute("aria-hidden", "true");
  }

  async function onStop() {
    if (!currentTaskId) return;
    appendLog("Stopping task...");
    try {
      const res = await fetch(`/api/tasks/${currentTaskId}/stop`, { method: "POST" });
      const data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || "停止失败");
      setGlobalStatus("已停止", "failed");
      showToast("任务已停止", "error");
    } catch (e) {
      showToast(e.message, "error");
    }
  }

  function connectSSE(taskId) {
    if (evtSource) evtSource.close();
    stopRecoveryPolling();
    evtSource = new EventSource(`/api/tasks/${taskId}/stream`);

    evtSource.addEventListener("log", (e) => {
      appendLog(JSON.parse(e.data));
    });

    evtSource.addEventListener("step", (e) => {
      const step = JSON.parse(e.data);
      updateSteps(step);
      setGlobalStatus(`${STEP_LABELS[step] || step}中`, "running");
    });

    evtSource.addEventListener("heartbeat", (e) => {
      const info = JSON.parse(e.data);
      if (info.step) updateSteps(info.step);
      setGlobalStatus(`${STEP_LABELS[info.step] || "任务"}中`, "running");
    });

    evtSource.addEventListener("done", (e) => {
      const info = JSON.parse(e.data);
      evtSource.close();
      evtSource = null;
      stopRecoveryPolling();
      setRunningState(false);

      if (info.status === "done") {
        markAllStepsDone();
        setGlobalStatus("完成");
        showNotice(`任务 ${taskId} 已完成，可审查并下载交付包`, "success");
        showToast("生成任务已完成", "success");
        loadResults(taskId);
      } else {
        markStepFailed();
        setGlobalStatus(info.status === "stopped" ? "已停止" : "失败", "failed");
        showNotice(`任务 ${taskId} ${statusText(info.status)}，已生成的截图和报告可在产物审查中查看`, "error");
        showToast(`任务${statusText(info.status)}，可查看产物`, "error");
        loadResults(taskId);
      }
      loadHistory();
    });

    evtSource.onerror = () => {
      if (evtSource) evtSource.close();
      evtSource = null;
      appendLog("--- log stream disconnected; switching to status polling ---");
      setGlobalStatus("恢复连接中", "running");
      showToast("日志连接中断，正在轮询任务状态", "error");
      startRecoveryPolling(taskId);
    };
  }

  function startRecoveryPolling(taskId) {
    stopRecoveryPolling();
    const poll = async () => {
      try {
        const res = await fetch(`/api/tasks/${taskId}`);
        const data = await safeJson(res);
        if (!res.ok) return;
        if (data.run_step) {
          updateSteps(data.run_step);
          setGlobalStatus(`${STEP_LABELS[data.run_step] || "任务"}中`, "running");
        }
        if (["done", "failed", "stopped"].includes(data.run_status)) {
          stopRecoveryPolling();
          setRunningState(false);
          if (data.run_status === "done") {
            markAllStepsDone();
            setGlobalStatus("完成");
            showNotice(`任务 ${taskId} 已完成，可审查并下载交付包`, "success");
            showToast("生成任务已完成", "success");
          } else {
            markStepFailed();
            setGlobalStatus(data.run_status === "stopped" ? "已停止" : "失败", "failed");
            showNotice(`任务 ${taskId} ${statusText(data.run_status)}，已生成的产物可查看`, "error");
            showToast(`任务${statusText(data.run_status)}，可查看产物`, "error");
          }
          await loadResults(taskId);
          await loadHistory();
        }
      } catch (e) {
        // Keep polling. Network can flap while the worker continues.
      }
    };
    poll();
    recoveryTimer = window.setInterval(poll, 3000);
  }

  function stopRecoveryPolling() {
    if (recoveryTimer) {
      window.clearInterval(recoveryTimer);
      recoveryTimer = null;
    }
  }

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
      if (idx >= 0 && si < idx) el.classList.add("done");
      else if (si === idx) el.classList.add("active");
    }
  }

  function markAllStepsDone() {
    for (const el of $$(".step")) {
      el.classList.remove("active", "failed");
      el.classList.add("done");
    }
  }

  function markStepFailed() {
    const active = $(".step.active");
    if (active) {
      active.classList.remove("active");
      active.classList.add("failed");
      return;
    }
    const first = $(".step:not(.done)");
    if (first) first.classList.add("failed");
  }

  async function loadResults(taskId) {
    try {
      const res = await fetch(`/api/tasks/${taskId}`);
      const data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || "任务不存在");

      currentTaskId = taskId;
      syncProfileFromTask(data.task);
      setTaskContext(taskId, data.task);
      if (data.analysis && data.task && data.task.analysis && data.task.analysis.confirmed === false) {
        pendingAnalysis = data.analysis;
        showAnalysisModal(pendingAnalysis);
        showNotice("该任务已完成页面分析，确认后可继续生成。", "success");
      }
      setDownloadEnabled(Boolean(data.artifacts && Object.values(data.artifacts).some(Boolean)));

      const setImg = (sel, name) => {
        const img = $(sel);
        const fig = img.closest("figure");
        if (data.artifacts && data.artifacts[name]) {
          img.src = `/api/tasks/${taskId}/artifacts/${name}?t=${Date.now()}`;
          img.tabIndex = 0;
          fig.classList.add("visible");
        } else {
          fig.classList.remove("visible");
          img.removeAttribute("src");
          img.removeAttribute("tabindex");
        }
      };
      setImg("#img-ref", "current.png");
      setImg("#img-actual", "full.png");
      setImg("#img-diff", "diff.png");

      if (data.artifacts && data.artifacts["report.json"]) {
        const rr = await fetch(`/api/tasks/${taskId}/artifacts/report.json?t=${Date.now()}`);
        const report = await rr.json();
        renderReport(report);
      } else {
        $("#report-summary").innerHTML = `
          <div class="empty-state">
            <strong>任务已加载，但暂无 report.json</strong>
            <span>如果任务未完成或缺少参考图，视觉验证报告可能不会生成。</span>
          </div>
        `;
      }

      $("#download-btn").onclick = () => {
        window.location = `/api/tasks/${taskId}/export`;
      };

      const codeRes = await fetch(`/api/tasks/${taskId}/code`);
      const codeData = await safeJson(codeRes);
      const files = Object.entries(codeData.files || {});
      $("#code-block").textContent = files.length
        ? files.map(([name, content]) => `// --- ${name} ---\n${content}`).join("\n\n")
        : "// 暂无生成代码";

      highlightTask(taskId);
      document.getElementById("result-panel").scrollIntoView({
        behavior: prefersReducedMotion() ? "auto" : "smooth",
        block: "start",
      });
    } catch (e) {
      showToast(e.message, "error");
    }
  }

  function renderReport(report) {
    const passed = report.pass === true || report.passed === true;
    if (report.manual_review_required) {
      $("#report-summary").innerHTML = `
      <div class="metric-grid">
        <div class="metric-card">
          <span class="metric-label">验证状态</span>
          <span class="metric-value pass">待人工审查</span>
          </div>
          <div class="metric-card">
            <span class="metric-label">验证方式</span>
            <span class="metric-value">${escapeHtml(validationModeText(report.validation_mode || "manual_review"))}</span>
          </div>
          <div class="metric-card">
            <span class="metric-label">页面类型</span>
            <span class="metric-value">${escapeHtml(pageTypeText(report.analysis && report.analysis.page_type))}</span>
          </div>
        </div>
      `;
      return;
    }
    $("#report-summary").innerHTML = `
      <div class="metric-grid">
        <div class="metric-card">
          <span class="metric-label">验证状态</span>
          <span class="metric-value ${passed ? "pass" : "fail"}">${passed ? "通过" : "未通过"}</span>
        </div>
        <div class="metric-card">
          <span class="metric-label">Diff Ratio</span>
          <span class="metric-value">${num(report.diff_ratio, 4)}</span>
        </div>
        <div class="metric-card">
          <span class="metric-label">Mean Abs Diff</span>
          <span class="metric-value">${num(report.mean_abs_diff, 2)}</span>
        </div>
      </div>
    `;
  }

  function resetResults() {
    $("#report-summary").innerHTML = `
      <div class="empty-state">
        <strong>任务运行中</strong>
        <span>生成完成后会刷新截图、验证指标和代码预览。</span>
      </div>
    `;
    $$(".image-compare figure").forEach((fig) => fig.classList.remove("visible"));
    $("#code-block").textContent = "";
    $("#code-block").classList.remove("visible");
    $(".code-toggle").textContent = "查看生成代码";
  }

  function showAnalysisModal(analysis) {
    pendingAnalysis = analysis || {};
    $("#analysis-validation-mode").value = pendingAnalysis.validation_mode || "manual_review";
    renderAnalysisSummary(pendingAnalysis);
    renderAnalysisElements(pendingAnalysis.elements || []);
    renderAnalysisInteractions(pendingAnalysis.interactions || []);
    renderAnalysisStates(pendingAnalysis.states || []);
    const modal = $("#analysis-modal");
    modal.classList.add("open");
    modal.setAttribute("aria-hidden", "false");
    $("#analysis-confirm-btn").focus({ preventScroll: true });
  }

  function closeAnalysisModal() {
    const modal = $("#analysis-modal");
    modal.classList.remove("open");
    modal.setAttribute("aria-hidden", "true");
  }

  function renderAnalysisSummary(analysis) {
    const containsImages = analysis.contains_images ? "是" : "否";
    const risks = Array.isArray(analysis.risk_notes) && analysis.risk_notes.length
      ? analysis.risk_notes.slice(0, 4).map((x) => `<li>${escapeHtml(localizeAnalysisText(x))}</li>`).join("")
      : "<li>暂无额外风险提示</li>";
    const layoutNotes = Array.isArray(analysis.layout_notes)
      ? analysis.layout_notes.map(localizeAnalysisText).join(" ")
      : localizeAnalysisText(analysis.layout_notes || "暂无布局备注");
    $("#analysis-summary").innerHTML = `
      <div class="metric-grid analysis-metrics">
        <div class="metric-card">
          <span class="metric-label">页面类型</span>
          <span class="metric-value">${escapeHtml(pageTypeText(analysis.page_type))}</span>
        </div>
        <div class="metric-card">
          <span class="metric-label">包含图片</span>
          <span class="metric-value">${containsImages}</span>
        </div>
        <div class="metric-card">
          <span class="metric-label">推荐验证</span>
          <span class="metric-value">${escapeHtml(validationModeText(analysis.validation_mode))}</span>
        </div>
      </div>
      <p class="analysis-notes">${escapeHtml(layoutNotes)}</p>
      <ul class="analysis-risks">${risks}</ul>
    `;
  }

  function renderAnalysisElements(elements) {
    const list = elements.slice(0, 24).map((el) => `
      <li>
        <span>${escapeHtml(elementTypeText(el.type))}</span>
        <strong>${escapeHtml(localizeAnalysisText(el.content || el.role || ""))}</strong>
        <em>${escapeHtml(priorityText(el.priority))}</em>
      </li>
    `).join("");
    $("#analysis-elements").innerHTML = list
      ? `<h4>识别元素</h4><ul>${list}</ul>`
      : `<div class="empty-state"><strong>未识别到明确元素</strong><span>仍可确认后继续生成。</span></div>`;
  }

  function renderAnalysisInteractions(interactions) {
    const list = interactions.slice(0, 32).map((item) => `
      <li>
        <span>${escapeHtml(interactionTypeText(item.type))}</span>
        <strong>${escapeHtml(localizeAnalysisText(item.label || item.role || item.expected_behavior || ""))}</strong>
        <em>${escapeHtml(widgetText(item.lvgl_widget))}</em>
      </li>
    `).join("");
    $("#analysis-interactions").innerHTML = list
      ? `<h4>交互控件</h4><ul>${list}</ul>`
      : `<div class="empty-state"><strong>未识别到交互控件</strong><span>如页面中有按键、滑动、菜单或图标点击，请返回调整输入或后续手动补充。</span></div>`;
  }

  function renderAnalysisStates(states) {
    const list = states.slice(0, 20).map((item) => `
      <li>
        <span>${escapeHtml(localizeAnalysisText(item.name || "状态"))}</span>
        <strong>${escapeHtml(localizeAnalysisText(item.description || item.value || ""))}</strong>
        <em>${escapeHtml(priorityText(item.priority))}</em>
      </li>
    `).join("");
    $("#analysis-states").innerHTML = list
      ? `<h4>动态状态</h4><ul>${list}</ul>`
      : "";
  }

  async function loadHistory() {
    const res = await fetch("/api/tasks");
    if (!res.ok) throw new Error("无法加载历史任务");
    const tasks = await res.json();
    const list = $("#task-list");
    list.innerHTML = "";

    if (!tasks.length) {
      const li = document.createElement("li");
      li.innerHTML = `
        <span class="task-title">还没有任务</span>
        <span class="task-subtitle">创建第一个 HTML、图片或 URL 生成任务。</span>
      `;
      list.appendChild(li);
      return;
    }

    for (const t of tasks) {
      const li = document.createElement("li");
      li.dataset.taskId = t.task_id;
      if (t.task_id === currentTaskId) li.classList.add("active");

      const title = document.createElement("span");
      title.className = "task-title";
      title.textContent = t.page_name || t.task_id;

      const status = document.createElement("span");
      status.className = `status ${t.status || "idle"}`;
      status.textContent = statusText(t.status || "idle");

      const subtitle = document.createElement("span");
      subtitle.className = "task-subtitle";
      subtitle.textContent = `${t.task_id} · ${sourceTypeText(t.source_type)} · ${taskTargetText(t.target)} · ${formatDate(t.created_at)}`;

      const deleteBtn = document.createElement("button");
      deleteBtn.className = "task-delete";
      deleteBtn.type = "button";
      deleteBtn.setAttribute("aria-label", `删除任务 ${t.page_name || t.task_id}`);
      deleteBtn.title = "删除";
      deleteBtn.innerHTML = `
        <svg viewBox="0 0 24 24" focusable="false" aria-hidden="true">
          <path d="M5 7h14M10 11v6M14 11v6M9 7l1-2h4l1 2M7 7l1 13h8l1-13"/>
        </svg>
      `;
      deleteBtn.addEventListener("click", (e) => {
        e.stopPropagation();
        deleteTask(t.task_id, t.page_name || t.task_id);
      });

      li.appendChild(title);
      li.appendChild(status);
      li.appendChild(subtitle);
      li.appendChild(deleteBtn);
      li.addEventListener("click", () => loadResults(t.task_id));
      list.appendChild(li);
    }
  }

  async function deleteTask(taskId, label) {
    const ok = window.confirm(`删除历史任务“${label}”？\n\n该操作会移除输入文件、生成代码、截图、报告和导出包。`);
    if (!ok) return;

    try {
      const res = await fetch(`/api/tasks/${taskId}`, { method: "DELETE" });
      const data = await safeJson(res);
      if (!res.ok) throw new Error(data.error || "删除失败");

      if (currentTaskId === taskId) {
        currentTaskId = null;
        setTaskContext(null);
        resetProgress();
        resetResults();
        setDownloadEnabled(false);
      }

      showToast(`任务 ${taskId} 已删除`, "success");
      await loadHistory();
    } catch (e) {
      showToast(e.message, "error");
    }
  }

  function setRunningState(running) {
    $("#start-btn").disabled = running;
    $("#stop-btn").style.display = running ? "inline-flex" : "none";
    setGlobalStatus(running ? "运行中" : "就绪", running ? "running" : "");
  }

  function setAwaitingConfirmationState() {
    $("#start-btn").disabled = true;
    $("#stop-btn").style.display = "none";
    setGlobalStatus("待确认");
  }

  function setGlobalStatus(text, state) {
    const el = $("#global-status");
    el.textContent = text;
    el.classList.remove("running", "failed");
    if (state) el.classList.add(state);
  }

  function setTaskContext(taskId, task) {
    const target = task ? taskTargetText(task.target) : "";
    $("#task-context").textContent = taskId ? `任务 ${taskId}${target ? ` · ${target}` : ""}` : "未选择任务";
  }

  function setDownloadEnabled(enabled) {
    const btn = $("#download-btn");
    btn.disabled = !enabled;
    btn.setAttribute("aria-disabled", String(!enabled));
    if (!enabled) btn.onclick = null;
  }

  function appendLog(line) {
    const logBox = $(".log-box");
    logBox.classList.add("visible");
    logBox.textContent += line + "\n";
    logBox.scrollTop = logBox.scrollHeight;
  }

  function showNotice(message, type) {
    const notice = $("#form-notice");
    notice.textContent = message;
    notice.className = `notice visible ${type || ""}`.trim();
    notice.setAttribute("role", type === "error" ? "alert" : "status");
  }

  function clearNotice() {
    const notice = $("#form-notice");
    notice.textContent = "";
    notice.className = "notice";
    notice.setAttribute("role", "status");
  }

  function focusInvalid(targetId) {
    if (!targetId) return;
    const target = document.getElementById(targetId);
    if (!target) return;
    target.setAttribute("aria-invalid", "true");
    if (typeof target.focus === "function") {
      target.focus({ preventScroll: true });
    }
    target.scrollIntoView({
      behavior: prefersReducedMotion() ? "auto" : "smooth",
      block: "center",
    });
  }

  function clearFieldErrors() {
    ["profile-select", "custom-width", "custom-height", "url-input", "html-drop-zone", "image-drop-zone"].forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.removeAttribute("aria-invalid");
    });
  }

  function showToast(message, type) {
    const stack = $("#toast-stack");
    const toast = document.createElement("div");
    toast.className = `toast ${type || ""}`.trim();
    toast.textContent = message;
    stack.appendChild(toast);
    setTimeout(() => {
      toast.remove();
    }, 4200);
  }

  function prefersReducedMotion() {
    return window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  }

  function highlightTask(taskId) {
    $$("#task-list li").forEach((li) => {
      li.classList.toggle("active", li.dataset.taskId === taskId);
    });
  }

  async function safeJson(res) {
    try {
      return await res.json();
    } catch (e) {
      return {};
    }
  }

  function formatSize(bytes) {
    if (bytes > 1024 * 1024) return (bytes / 1024 / 1024).toFixed(2) + " MB";
    return (bytes / 1024).toFixed(1) + " KB";
  }

  function fileKind(name) {
    const ext = (name.split(".").pop() || "").toUpperCase();
    if (!ext || ext.length > 4) return "文件";
    return ext;
  }

  function htmlFileRole(file) {
    return /\.html?$/i.test(file.name) ? "主 HTML" : "资源";
  }

  function imageFileRole(_file, index) {
    return index === 0 ? "主图" : "素材";
  }

  function num(value, digits) {
    const n = Number(value);
    return Number.isFinite(n) ? n.toFixed(digits) : "--";
  }

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#039;");
  }

  function statusText(status) {
    return {
      idle: "待机",
      running: "运行中",
      done: "完成",
      failed: "失败",
      stopped: "已停止",
    }[status] || status || "未知";
  }

  function sourceTypeText(sourceType) {
    return {
      html: "HTML",
      image: "图片",
      url: "URL",
      reference_only: "参考图",
    }[sourceType] || "未知";
  }

  function pageTypeText(value) {
    return {
      text_controls: "文字与控件",
      image_heavy: "图片为主",
      mixed: "图文混合",
      reference_only: "仅参考图",
    }[value] || value || "--";
  }

  function validationModeText(value) {
    return {
      pixel: "像素级对比",
      layout_semantic: "语义布局校验",
      hybrid: "混合校验",
      manual_review: "人工审查",
    }[value] || value || "--";
  }

  function priorityText(value) {
    return {
      high: "高",
      medium: "中",
      low: "低",
    }[value] || value || "中";
  }

  function elementTypeText(value) {
    return {
      text: "文本",
      button: "按钮",
      image: "图片",
      icon: "图标",
      icon_button: "图标按钮",
      floating_button: "悬浮按钮",
      floating_action_button: "悬浮按钮",
      card: "卡片",
      container: "容器",
      panel: "面板",
      tab_bar: "标签栏",
      tab_item: "标签项",
      scrollbar: "滚动条",
      input: "输入框",
      select: "选择框",
      textarea: "文本输入",
      h1: "标题",
      h2: "小标题",
      h3: "小标题",
      p: "段落",
      a: "链接",
      label: "标签",
    }[value] || value || "元素";
  }

  function interactionTypeText(value) {
    return {
      button: "按钮",
      icon_button: "图标按钮",
      tab: "标签切换",
      menu_item: "菜单项",
      dropdown: "下拉菜单",
      switch: "开关",
      slider: "滑块",
      checkbox: "复选框",
      radio: "单选项",
      text_input: "文本输入",
      link_button: "链接按钮",
      gesture_area: "手势区域",
      carousel: "轮播",
      modal_trigger: "弹窗入口",
      back_button: "返回按钮",
      next_button: "下一步按钮",
    }[value] || elementTypeText(value);
  }

  function widgetText(value) {
    return {
      lv_button: "按钮控件",
      lv_switch: "开关控件",
      lv_slider: "滑块控件",
      lv_dropdown: "下拉控件",
      lv_checkbox: "复选框控件",
      lv_textarea: "文本输入控件",
      lv_arc: "弧形控件",
      lv_roller: "滚轮控件",
      lv_bar: "进度条控件",
    }[value] || value || "待确认";
  }

  function localizeAnalysisText(value) {
    const text = String(value || "");
    const exact = {
      "Heuristic analysis based on HTML tags and uploaded asset files.": "已根据 HTML 标签和上传资源文件完成启发式分析。",
      "Image or asset content is not suitable for strict whole-screen pixel diff.": "页面包含图片或素材内容，不适合使用严格的整屏像素级对比。",
      "top navigation with active Profile tab": "顶部导航栏，Profile 标签处于激活状态",
      "active navigation tab": "当前激活的导航标签",
      "inactive navigation tab": "未激活的导航标签",
      "main content container": "主内容容器",
      "profile name heading": "个人资料名称标题",
      "profile description": "个人资料描述",
      "primary action": "主要操作",
      "secondary action": "次要操作",
      "primary action button": "主要操作按钮",
      "secondary action button": "次要操作按钮",
      "user avatar": "用户头像",
      "profile avatar": "个人资料头像",
      "contact method icon": "联系方式图标",
      "email address": "邮箱地址",
      "phone number": "电话号码",
      "vertical scroll indicator": "垂直滚动指示器",
      "page scroll position": "页面滚动位置",
      "floating quick action": "悬浮快捷操作",
      "floating action button": "悬浮操作按钮",
    };
    if (exact[text]) return exact[text];
    return text
      .replace(/\bViewport\b/g, "视口")
      .replace(/\blayout\b/gi, "布局")
      .replace(/\bprofile card\b/gi, "个人资料卡片")
      .replace(/\bProfile summary card\b/g, "个人资料摘要卡片")
      .replace(/\bCircular portrait photo\b/gi, "圆形头像照片")
      .replace(/\bcircular portrait photo\b/gi, "圆形头像照片")
      .replace(/\bemail envelope\b/gi, "邮箱图标")
      .replace(/\bphone handset\b/gi, "电话图标")
      .replace(/\bvertical scroll indicator\b/gi, "垂直滚动指示器")
      .replace(/\bdroplet icon\b/gi, "水滴图标")
      .replace(/\bdroplet\/flame icon\b/gi, "水滴/火焰图标")
      .replace(/\bbutton\b/gi, "按钮")
      .replace(/\bcard\b/gi, "卡片")
      .replace(/\bcontainer\b/gi, "容器")
      .replace(/\bimage\b/gi, "图片")
      .replace(/\bicon\b/gi, "图标")
      .replace(/\bactive\b/gi, "激活")
      .replace(/\binactive\b/gi, "未激活")
      .replace(/\bhigh\b/gi, "高")
      .replace(/\bmedium\b/gi, "中")
      .replace(/\blow\b/gi, "低");
  }

  function updateProfileSummary() {
    const summary = $("#profile-summary");
    const sel = $("#profile-select");
    if (!summary || !sel) return;

    if (sel.value === "__custom__") {
      const name = $("#custom-profile-name").value.trim() || "自定义平台";
      const width = $("#custom-width").value || "?";
      const height = $("#custom-height").value || "?";
      summary.textContent = `当前目标：${name} · ${width}x${height} · 32bit · 160dpi`;
      return;
    }

    const p = profilesData.find((x) => x.file === sel.value);
    if (!p) {
      summary.textContent = "请选择目标平台";
      return;
    }

    summary.textContent = `当前目标：${targetText({
      name: p.name,
      viewport: p.screen,
      color_depth: p.screen.color_depth,
      dpi: p.screen.dpi,
    })}`;
  }

  function syncProfileFromTask(task) {
    const profileRef = task && task.target && task.target.profile;
    const profileFile = basename(profileRef || "");
    const sel = $("#profile-select");
    if (profileFile && profilesData.some((p) => p.file === profileFile)) {
      sel.value = profileFile;
      onProfileChange();
    }
    updateProfileSummaryFromTarget(task && task.target, profileFile);
  }

  function updateProfileSummaryFromTarget(target, profileFile) {
    const summary = $("#profile-summary");
    if (!summary || !target) return;
    const p = profilesData.find((item) => item.file === profileFile);
    summary.textContent = `当前任务目标：${targetText({
      name: p ? p.name : basename(target.profile || profileFile || "unknown"),
      viewport: target.viewport,
      color_depth: target.color_depth,
      dpi: target.dpi,
    })}`;
  }

  function taskTargetText(target) {
    if (!target) return "未指定平台";
    return targetText(target);
  }

  function targetText(target) {
    const viewport = target.viewport || {};
    const width = viewport.width || "?";
    const height = viewport.height || "?";
    const name = target.name || target.file || "目标平台";
    const colorDepth = target.color_depth || viewport.color_depth;
    const dpi = target.dpi || viewport.dpi;
    const extras = [];
    if (colorDepth) extras.push(`${colorDepth}bit`);
    if (dpi) extras.push(`${dpi}dpi`);
    return `${name} ${width}x${height}${extras.length ? ` · ${extras.join(" · ")}` : ""}`;
  }

  function basename(path) {
    return String(path || "").split(/[\\/]/).pop();
  }

  function formatDate(seconds) {
    const ts = Number(seconds);
    if (!Number.isFinite(ts) || ts <= 0) return "未知时间";
    return new Intl.DateTimeFormat("zh-CN", {
      month: "2-digit",
      day: "2-digit",
      hour: "2-digit",
      minute: "2-digit",
    }).format(new Date(ts * 1000));
  }

  document.addEventListener("DOMContentLoaded", init);
})();
