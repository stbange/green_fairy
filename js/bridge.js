/* Green Fairy Web Runtime Bridge: External Asset HTTP Hydration + Mobile Touch + WebXR */
(function () {
  const canvas = document.getElementById("canvas");
  const loadingEl = document.getElementById("gf-loading");
  const statusMsg = document.getElementById("gf-status-msg");
  const modeBadge = document.getElementById("gf-mode-badge");
  const audioBtn = document.getElementById("gf-audio-btn");
  const xrBtn = document.getElementById("gf-xr-btn");

  let ready = false;
  let currentMode = 0; /* 0=desktop, 1=touch, 2=xr */
  let fetchedAssets = [];

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

  async function hydrateExternalAssets() {
    statusMsg.textContent = "Loading assets/manifest.json...";
    const res = await fetch("assets/manifest.json", { cache: "no-store" });
    if (!res.ok) throw new Error("Failed to fetch assets/manifest.json: " + res.status);
    const manifest = await res.json();
    ensureDir("/assets");

    const list = manifest.assets || [];
    fetchedAssets = [];
    await Promise.all(list.map(async (rel) => {
      const r = await fetch("assets/" + rel, { cache: "no-store" });
      if (!r.ok) throw new Error("Failed to fetch external asset: " + rel);
      const bytes = new Uint8Array(await r.arrayBuffer());
      const full = "/assets/" + rel;
      const slash = full.lastIndexOf("/");
      if (slash > 0) ensureDir(full.slice(0, slash));
      Module.FS.writeFile(full, bytes);
      fetchedAssets.push(rel);
    }));
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
        Module._gf_web_pointer(0, (t.identifier % 32767) + 10, x, y);
      }
    }, { passive: false });

    canvas.addEventListener("touchmove", (e) => {
      e.preventDefault();
      if (!ready) return;
      for (const t of e.changedTouches) {
        const [x, y] = canvasCoords(t.clientX, t.clientY);
        Module._gf_web_pointer(1, (t.identifier % 32767) + 10, x, y);
      }
    }, { passive: false });

    function onTouchEnd(e) {
      e.preventDefault();
      if (!ready) return;
      for (const t of e.changedTouches) {
        const [x, y] = canvasCoords(t.clientX, t.clientY);
        Module._gf_web_pointer(2, (t.identifier % 32767) + 10, x, y);
      }
    }
    canvas.addEventListener("touchend", onTouchEnd, { passive: false });
    canvas.addEventListener("touchcancel", onTouchEnd, { passive: false });

    let mouseDown = false;
    canvas.addEventListener("mousedown", (e) => {
      unlockAudio();
      mouseDown = true;
      if (!ready) return;
      const [x, y] = canvasCoords(e.clientX, e.clientY);
      Module._gf_web_pointer(0, 1, x, y);
    });
    window.addEventListener("mousemove", (e) => {
      if (!mouseDown || !ready) return;
      const [x, y] = canvasCoords(e.clientX, e.clientY);
      Module._gf_web_pointer(1, 1, x, y);
    });
    window.addEventListener("mouseup", (e) => {
      if (!mouseDown || !ready) return;
      mouseDown = false;
      const [x, y] = canvasCoords(e.clientX, e.clientY);
      Module._gf_web_pointer(2, 1, x, y);
    });

    window.addEventListener("keydown", (e) => {
      if (e.repeat || !ready) return;
      unlockAudio();
      if (e.key && e.key.length === 1) {
        Module._gf_web_key(e.key.charCodeAt(0), 1);
      }
    });
    window.addEventListener("keyup", (e) => {
      if (!ready) return;
      if (e.key && e.key.length === 1) {
        Module._gf_web_key(e.key.charCodeAt(0), 0);
      }
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
    getMode: () => modeLabel(currentMode).toLowerCase(),
    getStatus: () => {
      if (!ready || !Module._gf_web_get_status_json) return null;
      return JSON.parse(Module.UTF8ToString(Module._gf_web_get_status_json()));
    },
    setMode: (m) => syncMode(m === "xr" ? 2 : (m === "touch" ? 1 : 0)),
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
      } catch (err) {
        statusMsg.textContent = "Boot error: " + err.message;
        console.error(err);
      }
    },
  };
})();
