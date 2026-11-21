/* Green Fairy WebXR Immersive Session Manager */
window.GreenFairyXR = (function () {
  let activeSession = null;
  let refSpace = null;
  let viewMatPtr = 0;
  let projMatPtr = 0;
  let lastTs = 0;
  let prevTriggers = {};

  function getWebGLContext(canvas) {
    if (window.Module && window.Module.ctx) return window.Module.ctx;
    return canvas.getContext("webgl2", { xrCompatible: true }) || canvas.getContext("webgl2");
  }

  async function checkSupport(btn) {
    if (!navigator.xr || typeof navigator.xr.isSessionSupported !== "function") {
      btn.disabled = true;
      btn.textContent = "WEBXR N/A";
      return false;
    }
    try {
      const supported = await navigator.xr.isSessionSupported("immersive-vr");
      btn.disabled = !supported;
      btn.textContent = supported ? "ENTER WEBXR" : "WEBXR N/A";
      return supported;
    } catch (_) {
      btn.disabled = true;
      btn.textContent = "WEBXR N/A";
      return false;
    }
  }

  async function enter(canvas, btn, onStateChange) {
    if (activeSession || !navigator.xr) return false;
    const gl = getWebGLContext(canvas);
    if (gl && typeof gl.makeXRCompatible === "function") {
      try {
        await Promise.race([
          gl.makeXRCompatible(),
          new Promise((resolve) => setTimeout(resolve, 250)),
        ]);
      } catch (_) {
        /* Emulated or software WebGL2 backends may not support makeXRCompatible */
      }
    }
    const session = await navigator.xr.requestSession("immersive-vr");
    activeSession = session;
    prevTriggers = {};
    lastTs = 0;

    if (typeof XRWebGLLayer !== "undefined" && gl) {
      const layer = new XRWebGLLayer(session, gl);
      session.updateRenderState({ baseLayer: layer });
    }
    refSpace = await session.requestReferenceSpace("local");

    if (!viewMatPtr) viewMatPtr = Module._malloc(16 * 4);
    if (!projMatPtr) projMatPtr = Module._malloc(16 * 4);

    btn.textContent = "EXIT WEBXR";
    if (onStateChange) onStateChange(true);

    session.addEventListener("end", () => {
      activeSession = null;
      refSpace = null;
      if (Module._gf_webxr_exit) Module._gf_webxr_exit();
      btn.textContent = "ENTER WEBXR";
      if (onStateChange) onStateChange(false);
    });

    function onXRFrame(ts, frame) {
      if (!activeSession) return;
      session.requestAnimationFrame(onXRFrame);
      const dt = lastTs ? Math.min((ts - lastTs) / 1000.0, 0.05) : 0.011;
      lastTs = ts;

      const pose = frame.getViewerPose(refSpace);
      if (!pose) return;

      const baseLayer = session.renderState.baseLayer;
      if (gl && baseLayer && baseLayer.framebuffer !== undefined) {
        gl.bindFramebuffer(gl.FRAMEBUFFER, baseLayer.framebuffer);
        gl.clearColor(0.03, 0.06, 0.08, 1.0);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      }

      /* Forward XR controller trigger state */
      const sources = session.inputSources || [];
      for (let i = 0; i < sources.length; i++) {
        const gp = sources[i] && sources[i].gamepad;
        const trig = gp && gp.buttons && gp.buttons[0] ? (gp.buttons[0].value || (gp.buttons[0].pressed ? 1 : 0)) : 0;
        if (Math.abs((prevTriggers[i] || 0) - trig) > 0.25) {
          prevTriggers[i] = trig;
          Module._gf_webxr_controller_input(i + 1, trig);
        }
      }

      Module._gf_webxr_begin_frame(dt);
      const views = pose.views || [];
      for (let i = 0; i < views.length; i++) {
        const v = views[i];
        const vp = baseLayer && typeof baseLayer.getViewport === "function"
          ? baseLayer.getViewport(v)
          : { x: i * 480, y: 0, width: 480, height: 720 };
        const viewMat = v.transform && v.transform.inverse ? v.transform.inverse.matrix : new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]);
        const projMat = v.projectionMatrix || new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]);
        Module.HEAPF32.set(viewMat, viewMatPtr >> 2);
        Module.HEAPF32.set(projMat, projMatPtr >> 2);
        Module._gf_webxr_render_eye(viewMatPtr, projMatPtr, vp.x, vp.y, vp.width, vp.height);
      }
      Module._gf_webxr_end_frame();
    }

    session.requestAnimationFrame(onXRFrame);
    return true;
  }

  async function exit() {
    if (activeSession) {
      await activeSession.end();
    }
  }

  return { checkSupport, enter, exit, isActive: () => !!activeSession };
})();
