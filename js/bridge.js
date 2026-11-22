/* Green Fairy v2.0 Web Runtime Bridge: On-Demand Preset Hydration + 7-Room 3D + Mobile Touch + WebXR */
(function () {
  const canvas = document.getElementById("canvas");
  const loadingEl = document.getElementById("gf-loading");
  const statusMsg = document.getElementById("gf-status-msg");
  const modeBadge = document.getElementById("gf-mode-badge");
  const presetSelect = document.getElementById("gf-preset-select");
  const mutateBtn = document.getElementById("gf-mutate-btn");
  const audioBtn = document.getElementById("gf-audio-btn");
  const xrBtn = document.getElementById("gf-xr-btn");

  let ready = false;
  let currentMode = 0; /* 0=desktop, 1=touch, 2=xr */
  let fetchedAssets = [];
  let fetchedSet = new Set();
  let presetCatalog = [];

  function detectInputMode() {
    const params = new URLSearchParams(window.location.search);
    const forced = params.get("mode");
    if (forced === "touch") return 1;
    if (forced === "desktop") return 0;
    const coarse = window.matchMedia && window.matchMedia("(pointer: coarse)").matches;
    const hasTouch = (navigator.maxTouchPoints || 0) > 0 || ("ontouchstart" in window);
    const narrow = window.innerWidth <= 820;
    return (coarse || (hasTouch && narrow) || narrow) ? 1 : 0;
  }

  function modeLabel(m) {
    return m === 2 ? "WEBXR" : (m === 1 ? "TOUCH" : "DESKTOP");
  }

  function syncMode(m) {
    currentMode = m;
    modeBadge.textContent = "MODE: " + modeLabel(m);
    document.body.classList.toggle("gf-touch-mode", m === 1);
    if (ready && Module._gf_platform_set_input_mode) {
      Module._gf_platform_set_input_mode(m);
    }
  }

  function ensureDir(path) {
    const parts = path.split("/").filter(Boolean);
    let cur = "";
    for (let i = 0; i < parts.length; i++) {
      cur += "/" + parts[i];
      try { Module.FS.mkdir(cur); } catch (_) {}
    }
  }

  async function fetchAssetIntoFS(rel) {
    if (fetchedSet.has(rel)) return true;
    const r = await fetch("assets/" + rel, { cache: "no-cache" });
    if (!r.ok) throw new Error("Failed to fetch external asset: " + rel + " (" + r.status + ")");
    const bytes = new Uint8Array(await r.arrayBuffer());
    const full = "/assets/" + rel;
    const slash = full.lastIndexOf("/");
    if (slash > 0) ensureDir(full.slice(0, slash));
    Module.FS.writeFile(full, bytes);
    fetchedSet.add(rel);
    fetchedAssets.push(rel);
    return true;
  }

  async function hydrateExternalAssets() {
    statusMsg.textContent = "Loading assets/manifest.json...";
    const res = await fetch("assets/manifest.json", { cache: "no-cache" });
    if (!res.ok) throw new Error("Failed to fetch assets/manifest.json: " + res.status);
    const manifest = await res.json();
    ensureDir("/assets");

    const list = manifest.assets || [];
    fetchedAssets = [];
    fetchedSet = new Set();
    await Promise.all(list.map((rel) => fetchAssetIntoFS(rel)));

    /* Parse catalog.json if present to populate 320+ preset selector */
    try {
      const catBytes = Module.FS.readFile("/assets/patches/catalog.json", { encoding: "utf8" });
      const catJson = JSON.parse(catBytes);
      presetCatalog = catJson.presets || [];
    } catch (_) {
      presetCatalog = [];
    }

    if (presetSelect && presetCatalog.length > 0) {
      presetSelect.innerHTML = "";
      const byRealm = {};
      for (const p of presetCatalog) {
        if (!byRealm[p.realm]) byRealm[p.realm] = [];
        byRealm[p.realm].push(p);
      }
      for (const [realm, items] of Object.entries(byRealm)) {
        const grp = document.createElement("optgroup");
        grp.label = realm.replace(/_/g, " ").toUpperCase();
        for (const it of items) {
          const opt = document.createElement("option");
          opt.value = it.path;
          opt.textContent = it.name;
          grp.appendChild(opt);
        }
        presetSelect.appendChild(grp);
      }
    }
  }

  async function loadPresetOnDemand(relPath) {
    if (!ready || !relPath) return false;
    await fetchAssetIntoFS(relPath);
    const ptr = Module.stringToNewUTF8(relPath);
    const ok = Module._gf_web_apply_preset_path(ptr) === 1;
    Module._free(ptr);
    if (ok && presetSelect) presetSelect.value = relPath;
    return ok;
  }

  /* Gently warm remaining presets in small background batches after interactive ready */
  function scheduleBackgroundVaultWarmup() {
    let idx = 0;
    function stepBatch() {
      if (idx >= presetCatalog.length) return;
      const batch = presetCatalog.slice(idx, idx + 8);
      idx += 8;
      Promise.all(batch.map((p) => fetchAssetIntoFS(p.path).catch(() => {}))).finally(() => {
        setTimeout(stepBatch, 60);
      });
    }
    setTimeout(stepBatch, 600);
  }

  function unlockAudio() {
    if (ready && Module._gf_web_resume_audio) {
      Module._gf_web_resume_audio();
    }
    if (window.SDL3 && window.SDL3.audioContext && window.SDL3.audioContext.state === "suspended") {
      window.SDL3.audioContext.resume();
    }
  }

  function bindInputs() {
    function canvasCoords(clientX, clientY) {
      const rect = canvas.getBoundingClientRect();
      const sx = canvas.width / Math.max(rect.width, 1);
      const sy = canvas.height / Math.max(rect.height, 1);
      return [(clientX - rect.left) * sx, (clientY - rect.top) * sy];
    }

    canvas.addEventListener("touchstart", (e) => {
      e.preventDefault();
      unlockAudio();
      if (!ready) return;
      for (const t of e.changedTouches) {
        const [x, y] = canvasCoords(t.clientX, t.clientY);
        Module._gf_web_pointer_ex(0, (t.identifier % 32767) + 10, x, y, 0);
      }
    }, { passive: false });

    canvas.addEventListener("touchmove", (e) => {
      e.preventDefault();
      if (!ready) return;
      for (const t of e.changedTouches) {
        const [x, y] = canvasCoords(t.clientX, t.clientY);
        Module._gf_web_pointer_ex(1, (t.identifier % 32767) + 10, x, y, 0);
      }
    }, { passive: false });

    function onTouchEnd(e) {
      e.preventDefault();
      if (!ready) return;
      for (const t of e.changedTouches) {
        const [x, y] = canvasCoords(t.clientX, t.clientY);
        Module._gf_web_pointer_ex(2, (t.identifier % 32767) + 10, x, y, 0);
      }
    }
    canvas.addEventListener("touchend", onTouchEnd, { passive: false });
    canvas.addEventListener("touchcancel", onTouchEnd, { passive: false });

    let mouseButton = -1;
    canvas.addEventListener("mousedown", (e) => {
      unlockAudio();
      canvas.focus();
      mouseButton = e.button;
      if (!ready) return;
      const [x, y] = canvasCoords(e.clientX, e.clientY);
      const extra = e.button === 2 ? 2 : (e.shiftKey ? 1 : 0);
      Module._gf_web_pointer_ex(0, 1, x, y, extra);
    });
    window.addEventListener("mousemove", (e) => {
      if (!ready) return;
      const [x, y] = canvasCoords(e.clientX, e.clientY);
      const extra = mouseButton === 2 ? 2 : 0;
      Module._gf_web_pointer_ex(1, 1, x, y, extra);
    });
    window.addEventListener("mouseup", (e) => {
      if (mouseButton < 0 || !ready) return;
      mouseButton = -1;
      const [x, y] = canvasCoords(e.clientX, e.clientY);
      Module._gf_web_pointer_ex(2, 1, x, y, 0);
    });

    function mapKeyCode(e) {
      if (e.key === "Escape") return 27;
      if (e.key === "ArrowUp") return 119;
      if (e.key === "ArrowDown") return 115;
      if (e.key === "ArrowLeft") return 97;
      if (e.key === "ArrowRight") return 100;
      if (e.key && e.key.length === 1) return e.key.charCodeAt(0);
      return -1;
    }

    window.addEventListener("keydown", (e) => {
      if (e.repeat || !ready || document.activeElement === presetSelect) return;
      unlockAudio();
      const kc = mapKeyCode(e);
      if (kc > 0) Module._gf_web_key(kc, 1);
    });
    window.addEventListener("keyup", (e) => {
      if (!ready) return;
      const kc = mapKeyCode(e);
      if (kc > 0) Module._gf_web_key(kc, 0);
    });

    window.addEventListener("resize", () => {
      const w = Math.max(window.innerWidth, 320);
      const h = Math.max(window.innerHeight, 240);
      canvas.width = w;
      canvas.height = h;
      if (ready && Module._gf_web_resize) {
        Module._gf_web_resize(w, h);
      }
      if (!window.GreenFairyXR.isActive()) {
        syncMode(detectInputMode());
      }
    });

    if (presetSelect) {
      presetSelect.addEventListener("change", () => {
        unlockAudio();
        loadPresetOnDemand(presetSelect.value);
        canvas.focus();
      });
    }
    if (mutateBtn) {
      mutateBtn.addEventListener("click", () => {
        unlockAudio();
        window.GreenFairy.mutate("mutate", 0.32, 0.45);
      });
    }
    audioBtn.addEventListener("click", unlockAudio);
    xrBtn.addEventListener("click", async () => {
      unlockAudio();
      if (window.GreenFairyXR.isActive()) {
        await window.GreenFairyXR.exit();
      } else {
        await window.GreenFairyXR.enter(canvas, xrBtn, (inXR) => {
          syncMode(inXR ? 2 : detectInputMode());
        });
      }
    });
  }

  window.GreenFairy = {
    isReady: () => ready,
    getFetchedAssets: () => fetchedAssets.slice(),
    getCatalog: () => presetCatalog.slice(),
    getMode: () => modeLabel(currentMode).toLowerCase(),
    getStatus: () => {
      if (!ready || !Module._gf_web_get_status_json) return null;
      return JSON.parse(Module.UTF8ToString(Module._gf_web_get_status_json()));
    },
    setMode: (m) => syncMode(m === "xr" ? 2 : (m === "touch" ? 1 : 0)),
    teleportRoom: (r) => { if (ready) Module._gf_web_teleport_room(r); },
    focusRack: (id) => { if (ready) Module._gf_web_focus_rack(id); },
    exitFocus: () => { if (ready) Module._gf_web_exit_focus(); },
    loadPreset: (relPath) => loadPresetOnDemand(relPath),
    connectCable: (src, dst, gain = 0.8) => {
      if (!ready) return 0;
      const p0 = Module.stringToNewUTF8(src);
      const p1 = Module.stringToNewUTF8(dst);
      const cid = Module._gf_web_connect_cable(p0, p1, gain);
      Module._free(p0);
      Module._free(p1);
      return cid;
    },
    disconnectPort: (port) => {
      if (!ready) return 0;
      const p = Module.stringToNewUTF8(port);
      const n = Module._gf_web_disconnect_port(p);
      Module._free(p);
      return n;
    },
    mutate: (act = "mutate", depth = 0.32, rand = 0.45) => {
      if (!ready) return;
      const p = Module.stringToNewUTF8(act);
      Module._gf_web_mutator(p, depth, rand);
      Module._free(p);
    },
    noteOn: (n, v) => { unlockAudio(); if (ready) Module._gf_web_note_on(n, v ?? 0.85); },
    noteOff: (n) => { if (ready) Module._gf_web_note_off(n); },
    setXY: (x, y) => { if (ready) Module._gf_web_set_xy(x, y); },
    setVoxels: (en) => { if (ready) Module._gf_web_set_voxels(en ? 1 : 0); },
    enterXR: () => window.GreenFairyXR.enter(canvas, xrBtn, (inXR) => syncMode(inXR ? 2 : detectInputMode())),
    exitXR: () => window.GreenFairyXR.exit(),
  };

  window.Module = {
    canvas: canvas,
    onRuntimeInitialized: async function () {
      try {
        const w = Math.max(window.innerWidth, 320);
        const h = Math.max(window.innerHeight, 240);
        canvas.width = w;
        canvas.height = h;
        await hydrateExternalAssets();
        const mode = detectInputMode();
        const dirPtr = Module.stringToNewUTF8("/assets");
        const rc = Module._gf_web_start(dirPtr, w, h, mode);
        Module._free(dirPtr);
        if (rc !== 0) throw new Error("gf_web_start returned " + rc);
        bindInputs();
        await window.GreenFairyXR.checkSupport(xrBtn);
        loadingEl.style.display = "none";
        ready = true;
        syncMode(mode);
        scheduleBackgroundVaultWarmup();
      } catch (err) {
        statusMsg.textContent = "Boot error: " + err.message;
        console.error(err);
      }
    },
  };
})();
