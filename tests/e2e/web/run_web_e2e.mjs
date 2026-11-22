#!/usr/bin/env node
/**
 * Green Fairy v2.0 Automated Web, Mobile Touch & WebXR 7-Room E2E Suite (Playwright)
 */
import fs from "node:fs";
import http from "node:http";
import path from "node:path";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const { chromium } = require("playwright-core");

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".json": "application/json",
  ".lua": "text/plain; charset=utf-8",
  ".glsl": "text/plain; charset=utf-8",
  ".obj": "text/plain; charset=utf-8",
  ".gfvox": "text/plain; charset=utf-8",
  ".fnt": "text/plain; charset=utf-8",
  ".tga": "application/octet-stream",
  ".wav": "audio/wav",
};

function startStaticServer(rootDir) {
  const httpLog = [];
  return new Promise((resolve) => {
    const server = http.createServer((req, res) => {
      const cleanUrl = (req.url || "/").split("?")[0];
      httpLog.push(cleanUrl);
      const rel = cleanUrl === "/" ? "green_fairy.html" : cleanUrl.replace(/^\/+/, "");
      const full = path.resolve(rootDir, rel);
      if (!full.startsWith(path.resolve(rootDir)) || !fs.existsSync(full)) {
        res.writeHead(404);
        res.end("Not found");
        return;
      }
      const ext = path.extname(full);
      res.writeHead(200, { "Content-Type": MIME[ext] || "application/octet-stream" });
      fs.createReadStream(full).pipe(res);
    });
    server.listen(0, "127.0.0.1", () => {
      const { port } = server.address();
      resolve({ server, url: `http://127.0.0.1:${port}/green_fairy.html`, httpLog });
    });
  });
}

function assert(cond, msg) {
  if (!cond) throw new Error("E2E ASSERT FAILED: " + msg);
}

const WEBXR_MOCK_INIT = () => {
  window.__xrStats = { sessions: 0, frames: 0, ended: 0 };
  class MockXRWebGLLayer {
    constructor(session, gl) {
      this.session = session;
      this.framebuffer = null;
      this.framebufferWidth = gl.drawingBufferWidth || 960;
      this.framebufferHeight = gl.drawingBufferHeight || 720;
    }
    getViewport(view) {
      const half = Math.floor(this.framebufferWidth / 2);
      return { x: view.eye === "right" ? half : 0, y: 0, width: half, height: this.framebufferHeight };
    }
  }
  window.XRWebGLLayer = MockXRWebGLLayer;
  const idMat = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,-4,1]);
  const projMat = new Float32Array([1.2,0,0,0, 0,1.6,0,0, 0,0,-1.02,-1, 0,0,-0.2,0]);

  Object.defineProperty(navigator, "xr", {
    configurable: true,
    value: {
      async isSessionSupported(mode) {
        return mode === "immersive-vr";
      },
      async requestSession(mode) {
        if (mode !== "immersive-vr") throw new Error("Unsupported XR mode");
        window.__xrStats.sessions++;
        let alive = true;
        const listeners = {};
        const ctrl = {
          rayOrigin: [0, 1.5, 0],
          rayDir: [0, 0, -1],
          teleportRoom: 0,
          gamepad: { buttons: [{ pressed: false, value: 0 }, { pressed: false, value: 0 }] },
        };
        const session = {
          renderState: { baseLayer: null },
          inputSources: [ctrl],
          updateRenderState(st) { Object.assign(this.renderState, st); },
          async requestReferenceSpace() { return {}; },
          addEventListener(ev, fn) { (listeners[ev] ||= []).push(fn); },
          requestAnimationFrame(cb) {
            if (!alive) return 0;
            return window.requestAnimationFrame((ts) => {
              if (!alive) return;
              const f = ++window.__xrStats.frames;
              /*
               * Frame 2: Teleport XR player into Room 1 (Oscillator Conservatory, spawn at (0, 1.62, -14.8), yaw=0).
               * Osc 2 center rack sits at world (0, 1.54, -22.55):
               *   - osc2.audio_out jack is at local (-0.575, -0.74) => world (-0.575, 0.80, -22.534)
               *   - osc2.fm_in jack is at local (-0.115, -0.74)     => world (-0.115, 0.80, -22.534)
               * In Room 1 local stage coordinates (origin = (0, 0, -14.8)):
               *   - rayOrigin (-0.575, 0.80, -5.0) shooting (0, 0, -1) hits osc2.audio_out!
               *   - rayOrigin (-0.115, 0.80, -5.0) shooting (0, 0, -1) hits osc2.fm_in!
               */
              ctrl.teleportRoom = (f === 2) ? 1 : 0;
              if (f === 4) {
                ctrl.rayOrigin = [-0.575, 0.80, -5.0];
                ctrl.rayDir = [0, 0, -1];
                ctrl.gamepad.buttons[0] = { pressed: true, value: 1.0 };
              } else if (f === 5 || f === 6) {
                ctrl.rayOrigin = [-0.115, 0.80, -5.0];
                ctrl.rayDir = [0, 0, -1];
                ctrl.gamepad.buttons[0] = { pressed: true, value: 1.0 };
              } else if (f === 7) {
                ctrl.rayOrigin = [-0.115, 0.80, -5.0];
                ctrl.rayDir = [0, 0, -1];
                ctrl.gamepad.buttons[0] = { pressed: false, value: 0.0 };
              } else {
                ctrl.gamepad.buttons[0] = { pressed: false, value: 0.0 };
              }

              const frame = {
                getViewerPose() {
                  return {
                    views: [
                      { eye: "left",  transform: { inverse: { matrix: idMat } }, projectionMatrix: projMat },
                      { eye: "right", transform: { inverse: { matrix: idMat } }, projectionMatrix: projMat },
                    ],
                  };
                },
              };
              cb(ts, frame);
            });
          },
          async end() {
            if (!alive) return;
            alive = false;
            window.__xrStats.ended++;
            for (const fn of (listeners.end || [])) fn();
          },
        };
        return session;
      },
    },
  });
};

async function main() {
  const buildDir = path.resolve(process.argv[2] || "build/web-emscripten");
  assert(fs.existsSync(path.join(buildDir, "green_fairy.html")), `Missing ${buildDir}/green_fairy.html`);
  assert(fs.existsSync(path.join(buildDir, "assets", "manifest.json")), "Missing sibling assets/manifest.json");
  assert(fs.existsSync(path.join(buildDir, "assets", "patches", "catalog.json")), "Missing sibling assets/patches/catalog.json");

  const { server, url, httpLog } = await startStaticServer(buildDir);
  const chromePath = process.env.CHROMIUM_BIN || "/usr/bin/chromium";
  const browser = await chromium.launch({
    executablePath: fs.existsSync(chromePath) ? chromePath : undefined,
    headless: true,
    args: [
      "--no-sandbox",
      "--use-gl=angle",
      "--use-angle=swiftshader",
      "--enable-webgl",
      "--ignore-gpu-blocklist",
    ],
  });

  try {
    // ---------------- Scenario 1: Desktop 7-Room Navigation, On-Demand 320-Preset Fetch & Cable Patching ----------------
    {
      const ctx = await browser.newContext({ viewport: { width: 1280, height: 720 } });
      const page = await ctx.newPage();
      const errors = [];
      page.on("pageerror", (e) => errors.push(e.message));
      page.on("console", (m) => { if (m.type() === "error") errors.push(m.text()); });

      const t0 = Date.now();
      await page.goto(url, { waitUntil: "domcontentloaded" });
      await page.waitForFunction(() => window.GreenFairy && window.GreenFairy.isReady(), null, { timeout: 20000 });
      const bootMs = Date.now() - t0;

      const bootInfo = await page.evaluate(() => ({
        fetchedCount: window.GreenFairy.getFetchedAssets().length,
        catalogCount: window.GreenFairy.getCatalog().length,
        mode: window.GreenFairy.getMode(),
        status: window.GreenFairy.getStatus(),
      }));
      assert(bootInfo.mode === "desktop", `Expected desktop mode, got ${bootInfo.mode}`);
      assert(bootInfo.catalogCount === 320, `Expected 320 presets in catalog, got ${bootInfo.catalogCount}`);
      assert(bootInfo.fetchedCount < 120, `Boot should not eagerly block on all 320 presets (fetched ${bootInfo.fetchedCount})`);

      // On-demand HTTP preset loading across Deep Space & Astral Choirs realms
      const okSpace = await page.evaluate(() => window.GreenFairy.loadPreset("patches/deep_space/05_oort_cloud_dust.lua"));
      assert(okSpace, "On-demand loadPreset for Deep Space failed");
      let st = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(st.realm === "deep_space" && st.preset.includes("Oort Cloud"), `Unexpected preset status: ${JSON.stringify(st)}`);
      assert(httpLog.some((u) => u.includes("patches/deep_space/05_oort_cloud_dust.lua")), "Preset was not fetched via HTTP");

      const okChoir = await page.evaluate(() => window.GreenFairy.loadPreset("patches/astral_choirs/04_starlight_soprano_veil.lua"));
      assert(okChoir, "On-demand loadPreset for Astral Choirs failed");

      // Navigate across rooms (1..7 quick jump + WASD walk) and verify frustum culling
      await page.keyboard.press("1");
      await page.waitForTimeout(120);
      st = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(st.room === 1, `Expected room 1 after key '1', got ${st.room}`);
      assert(st.visibleRooms < 7, `Expected room frustum culling (<7 visible rooms), got ${st.visibleRooms}`);

      // Focus onto Rack 0 and unfocus via Escape
      await page.evaluate(() => window.GreenFairy.focusRack(0));
      await page.waitForTimeout(200);
      st = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(st.camMode === "focus" && st.focusedRack === 0, `Expected Rack-Focus on rack 0, got ${JSON.stringify(st)}`);

      await page.keyboard.press("Escape");
      await page.waitForTimeout(150);
      st = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(st.camMode === "walk", `Expected return to Free-Walk after Escape, got ${st.camMode}`);

      // Live modular cable unplugging & cross-room re-patching + Mutator + polyphonic notes
      const cablesBefore = st.cableCount;
      const removed = await page.evaluate(() => window.GreenFairy.disconnectPort("flt2.in"));
      assert(removed >= 1, "Expected >=1 cable disconnected from flt2.in");
      st = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(st.cableCount === cablesBefore - removed, "Cable count did not decrease on disconnect");

      const newCid = await page.evaluate(() => window.GreenFairy.connectCable("osc2.audio_out", "reverb.in", 0.75));
      assert(newCid > 0, "Expected valid cable ID when connecting osc2.audio_out -> reverb.in");
      await page.click("#gf-mutate-btn");
      await page.keyboard.down("z");
      await page.keyboard.down("c");
      await page.waitForTimeout(250);
      st = await page.evaluate(() => window.GreenFairy.getStatus());
      await page.keyboard.up("z");
      await page.keyboard.up("c");

      assert(st.activeVoices >= 2, `Expected >=2 active voices, got ${st.activeVoices}`);
      assert(st.drawCalls >= 10, `Expected rich 3D room drawCalls >= 10, got ${st.drawCalls}`);

      const shot = await page.screenshot();
      assert(shot.length > 4000, `Canvas screenshot unexpectedly small (${shot.length} bytes)`);
      assert(errors.length === 0, `Desktop console/page errors: ${errors.join("; ")}`);
      console.log(`PASS [Web Desktop 7-Room]: bootMs=${bootMs}ms, catalog=${bootInfo.catalogCount}, cables=${st.cableCount}, drawCalls=${st.drawCalls}`);
      await ctx.close();
    }

    // ---------------- Scenario 2: Mobile Touch 3D Walk/Look, Rack Focus & Multi-Touch Notes ----------------
    {
      const ctx = await browser.newContext({
        viewport: { width: 390, height: 844 },
        hasTouch: true,
        isMobile: true,
      });
      const page = await ctx.newPage();
      const errors = [];
      page.on("pageerror", (e) => errors.push(e.message));
      page.on("console", (m) => { if (m.type() === "error") errors.push(m.text()); });

      await page.goto(url, { waitUntil: "domcontentloaded" });
      await page.waitForFunction(() => window.GreenFairy && window.GreenFairy.isReady(), null, { timeout: 20000 });
      await page.waitForTimeout(250);

      const mode = await page.evaluate(() => window.GreenFairy.getMode());
      const badge = await page.locator("#gf-mode-badge").textContent();
      assert(mode === "touch" && badge.includes("TOUCH"), `Expected mobile touch mode, got mode=${mode}, badge=${badge}`);

      // Dispatch simultaneous 2-finger multi-touch chord on bottom keyboard + virtual walk pad drag
      await page.evaluate(() => {
        const c = document.getElementById("canvas");
        const mkTouch = (id, x, y) => new Touch({ identifier: id, target: c, clientX: x, clientY: y, pageX: x, pageY: y });
        const t1 = mkTouch(1, 55, 790);
        const t2 = mkTouch(2, 235, 790);
        const tw = mkTouch(3, 65, 620);
        c.dispatchEvent(new TouchEvent("touchstart", { touches: [t1, t2, tw], changedTouches: [t1, t2, tw], cancelable: true, bubbles: true }));
        const twMove = mkTouch(3, 65, 575);
        c.dispatchEvent(new TouchEvent("touchmove", { touches: [t1, t2, twMove], changedTouches: [twMove], cancelable: true, bubbles: true }));
      });
      await page.waitForTimeout(220);

      // Release walk thumb while keeping multi-touch chord fingers 1 & 2 held, then trigger Rack-Focus!
      await page.evaluate(() => {
        const c = document.getElementById("canvas");
        const mkTouch = (id, x, y) => new Touch({ identifier: id, target: c, clientX: x, clientY: y, pageX: x, pageY: y });
        const t1 = mkTouch(1, 55, 790);
        const t2 = mkTouch(2, 235, 790);
        const twEnd = mkTouch(3, 65, 575);
        c.dispatchEvent(new TouchEvent("touchend", { touches: [t1, t2], changedTouches: [twEnd], cancelable: true, bubbles: true }));
      });

      // Focus rack on mobile & inspect status
      await page.evaluate(() => window.GreenFairy.focusRack(0));
      await page.waitForTimeout(180);
      const st = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(st.mode === "touch", `Expected status.mode == touch, got ${st.mode}`);
      assert(st.camMode === "focus", `Expected mobile rack focus active, got ${st.camMode}`);
      assert(st.activeVoices >= 2, `Expected >=2 simultaneous multi-touch voices, got ${st.activeVoices}`);
      assert(errors.length === 0, `Mobile console/page errors: ${errors.join("; ")}`);
      console.log(`PASS [Web Mobile Touch]: mode=${st.mode}, voices=${st.activeVoices}, camMode=${st.camMode}`);
      await ctx.close();
    }

    // ---------------- Scenario 3: WebXR Stereo 7-Room Teleport & 6DoF XR Ray Cable Patching ----------------
    {
      const ctx = await browser.newContext({ viewport: { width: 960, height: 720 } });
      await ctx.addInitScript(WEBXR_MOCK_INIT);
      const page = await ctx.newPage();
      const errors = [];
      page.on("pageerror", (e) => errors.push(e.message));
      page.on("console", (m) => { if (m.type() === "error") errors.push(m.text()); });

      await page.goto(url, { waitUntil: "domcontentloaded" });
      await page.waitForFunction(() => window.GreenFairy && window.GreenFairy.isReady(), null, { timeout: 20000 });
      await page.waitForFunction(() => !document.getElementById("gf-xr-btn").disabled, null, { timeout: 5000 });

      const preXR = await page.evaluate(() => window.GreenFairy.getStatus());

      await page.click("#gf-xr-btn");
      await page.waitForFunction(() => window.__xrStats && window.__xrStats.frames >= 9, null, { timeout: 10000 });

      const stXR = await page.evaluate(() => window.GreenFairy.getStatus());
      assert(stXR.mode === "xr", `Expected XR mode active, got ${stXR.mode}`);
      assert(stXR.xrFrames >= 8, `Expected >=8 stereo XR frames rendered in C, got ${stXR.xrFrames}`);
      assert(stXR.room === 1, `Expected XR controller teleport to Room 1, got room ${stXR.room}`);
      assert(stXR.cableCount === preXR.cableCount + 1,
        `Expected 6DoF XR laser ray to patch +1 3D cable (${preXR.cableCount} -> ${stXR.cableCount})`);

      await page.evaluate(() => window.GreenFairy.exitXR());
      await page.waitForTimeout(200);
      const modeAfter = await page.evaluate(() => window.GreenFairy.getMode());
      assert(modeAfter === "desktop", `Expected return to desktop mode after XR exit, got ${modeAfter}`);
      assert(errors.length === 0, `WebXR console/page errors: ${errors.join("; ")}`);
      console.log(`PASS [WebXR Stereo 7-Room]: xrFrames=${stXR.xrFrames}, room=${stXR.room}, cables=${preXR.cableCount}->${stXR.cableCount}`);
      await ctx.close();
    }

    console.log("ALL WEB / MOBILE TOUCH / WEBXR 7-ROOM E2E TESTS PASSED!");
  } finally {
    await browser.close();
    server.close();
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
