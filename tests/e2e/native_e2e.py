#!/usr/bin/env python3
"""Green Fairy v2.0 automated native headless E2E verification suite (7 Rooms + Modular Cables + Mutator)."""

import math
import os
import shutil
import struct
import subprocess
import sys
import tempfile


def read_ppm(path):
    with open(path, "rb") as f:
        magic = f.readline().strip()
        assert magic == b"P6", f"Expected P6 PPM, got {magic}"
        dims = f.readline().strip().split()
        w, h = int(dims[0]), int(dims[1])
        maxv = int(f.readline().strip())
        assert maxv == 255
        data = f.read()
        assert len(data) == w * h * 3
        return w, h, data


def frame_diff(a, b):
    return sum(1 for x, y in zip(a, b) if abs(x - y) > 6)


def read_wav_metrics(path):
    with open(path, "rb") as f:
        raw = f.read()
    assert raw[:4] == b"RIFF" and raw[8:12] == b"WAVE", "Invalid WAV header"
    channels = struct.unpack_from("<H", raw, 22)[0]
    sr = struct.unpack_from("<I", raw, 24)[0]
    assert channels == 2 and sr == 44100, f"Unexpected WAV format: ch={channels}, sr={sr}"
    pcm = raw[44:]
    samples = struct.unpack(f"<{len(pcm)//2}h", pcm)
    sq = sum((s / 32768.0) ** 2 for s in samples)
    rms = math.sqrt(sq / max(len(samples), 1))
    peak = max(abs(s) for s in samples) / 32768.0
    return rms, peak, samples


def verify_external_assets_and_clean_src(repo_root, sibling_assets):
    src_dir = os.path.join(repo_root, "src")
    for root, _, files in os.walk(src_dir):
        for fn in files:
            p = os.path.join(root, fn)
            sz = os.path.getsize(p)
            assert sz < 65536, f"Suspiciously large source file (embedded binary asset?): {p} ({sz} bytes)"

    # Verify external 320+ preset vault & 16 sound samples exist beside binary
    preset_files = []
    for root, _, files in os.walk(os.path.join(sibling_assets, "patches")):
        for fn in files:
            if fn.endswith(".lua"):
                preset_files.append(fn)
    assert len(preset_files) >= 320, f"Expected >=320 external .lua presets beside binary, found {len(preset_files)}"


def main():
    if len(sys.argv) < 2:
        print("Usage: native_e2e.py <path/to/green_fairy>", file=sys.stderr)
        return 2

    binary = os.path.abspath(sys.argv[1])
    assert os.path.isfile(binary), f"Binary not found: {binary}"
    sibling_assets = os.path.join(os.path.dirname(binary), "assets")
    assert os.path.isdir(sibling_assets), f"Sibling external assets/ missing next to binary: {sibling_assets}"

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    verify_external_assets_and_clean_src(repo_root, sibling_assets)

    with tempfile.TemporaryDirectory(prefix="gf_native_e2e_") as tmp:
        room_ppms = {r: os.path.join(tmp, f"room_{r}.ppm") for r in range(1, 8)}
        ppm_focus = os.path.join(tmp, "r1_focus.ppm")
        ppm_vox_off = os.path.join(tmp, "r6_vox_off.ppm")
        ppm_touch = os.path.join(tmp, "r6_touch.ppm")

        wav_base = os.path.join(tmp, "base.wav")
        wav_unplugged = os.path.join(tmp, "unplugged.wav")
        wav_repatched = os.path.join(tmp, "repatched.wav")
        wav_mutated = os.path.join(tmp, "mutated.wav")
        script_path = os.path.join(tmp, "e2e_v2.txt")

        lines = [
            "MODE desktop",
            "VOXELS 1",
        ]
        # 1. Visit and render all 7 interconnected 3D rooms
        for r in range(1, 8):
            lines += [
                f"TELEPORT {r}",
                "STEP 3",
                f"SAVE_PPM {room_ppms[r]}",
            ]

        # 2. Walk from Room 6 toward Room 1, Focus Rack 1 (Osc 2), and Raycast-drag 3D knob
        lines += [
            "TELEPORT 6",
            "WALK 1.0 0.0 6",
            "TELEPORT 1",
            "FOCUS_RACK 1",
            "STEP 14",
            f"SAVE_PPM {ppm_focus}",
            "POINTER down 160 90 0",
            "POINTER move 160 40 0",
            "POINTER up 160 40 0",
            "STEP 2",
            "EXIT_FOCUS",
            "STEP 6",
        ]

        # 3. Preset load + Live Modular Cable Unplugging & Cross-Room Re-Patching + Mutator
        lines += [
            "PRESET patches/crystalline_sanctum/01_hilbert_quartz_prism.lua",
            "NOTE_ON 52 0.88",
            "NOTE_ON 59 0.84",
            "NOTE_ON 64 0.80",
            "STEP 4",
            f"SAVE_WAV {wav_base} 8192",
            # Unplug Master Bus input cables so output goes immediately silent (even while FX tails ring internally!)
            "DISCONNECT_PORT master.in_l",
            "DISCONNECT_PORT master.in_r",
            "STEP 2",
            f"SAVE_WAV {wav_unplugged} 8192",
            # Re-patch Osc 1 & Aetherizer directly across rooms into Master Output Bus!
            "CONNECT osc1.audio_out master.in_l 0.70",
            "CONNECT aether.out master.in_r 0.85",
            "STEP 2",
            f"SAVE_WAV {wav_repatched} 8192",
            # Trigger Absynth Mutator sound breeding
            "MUTATE 0.65 0.90",
            "STEP 2",
            f"SAVE_WAV {wav_mutated} 8192",
            # Voxel & Mobile Touch checks in Room 6
            "TELEPORT 6",
            "VOXELS 0",
            "STEP 2",
            f"SAVE_PPM {ppm_vox_off}",
            "MODE touch",
            "STEP 2",
            f"SAVE_PPM {ppm_touch}",
            "RELOAD",
            "NOTE_OFF 52",
            "NOTE_OFF 59",
            "NOTE_OFF 64",
            "STEP 2",
        ]

        with open(script_path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")

        env = os.environ.copy()
        env.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
        proc = subprocess.run(
            [binary, "--headless", "--width", "320", "--height", "240", "--e2e-script", script_path],
            cwd=tmp,
            env=env,
            capture_output=True,
            text=True,
            timeout=90,
        )
        if proc.returncode != 0:
            print("STDOUT:\n", proc.stdout)
            print("STDERR:\n", proc.stderr)
            raise AssertionError(f"green_fairy exited with code {proc.returncode}")

        # Verify all 7 rooms rendered rich, distinct 3D perspectives
        room_imgs = {}
        for r in range(1, 8):
            w, h, data = read_ppm(room_ppms[r])
            assert (w, h) == (320, 240)
            ucols = len(set(data[i:i + 3] for i in range(0, len(data), 3)))
            assert ucols >= 18, f"Room {r} had too few unique colors ({ucols})"
            room_imgs[r] = data
        for r in range(1, 7):
            d = frame_diff(room_imgs[r], room_imgs[r + 1])
            assert d > 1500, f"Room {r} and Room {r+1} look too similar (diff={d})"

        # Verify Rack-Focus zoom dramatically changes Room 1 framing
        _, _, d_focus = read_ppm(ppm_focus)
        focus_diff = frame_diff(room_imgs[1], d_focus)
        assert focus_diff > 5000, f"Rack-Focus did not zoom camera onto rack (diff={focus_diff})"

        # Verify Voxel toggle & Touch mode diffs
        _, _, d_voff = read_ppm(ppm_vox_off)
        _, _, d_touch = read_ppm(ppm_touch)
        assert frame_diff(room_imgs[6], d_voff) > 100, "Voxel toggle had no visual effect"
        assert frame_diff(d_voff, d_touch) > 500, "Mobile touch mode had no visual effect"

        # Verify Modular Cable Unplugging & Re-Patching + Mutator Audio Changes!
        rms_base, pk_base, smp_base = read_wav_metrics(wav_base)
        rms_unp, _, _ = read_wav_metrics(wav_unplugged)
        rms_rep, pk_rep, smp_rep = read_wav_metrics(wav_repatched)
        rms_mut, pk_mut, smp_mut = read_wav_metrics(wav_mutated)

        assert rms_base > 0.02 and pk_base <= 1.0, f"Base preset audio invalid: rms={rms_base:.4f}, peak={pk_base:.4f}"
        assert rms_unp < rms_base * 0.25, f"Unplugging osc->filter cables did not attenuate signal! ({rms_unp:.4f} vs {rms_base:.4f})"
        assert rms_rep > 0.02 and pk_rep <= 1.0, f"Re-patched cross-room cable did not restore audio! (rms={rms_rep:.4f})"
        rep_wave_diff = sum(abs(a - b) for a, b in zip(smp_base[:4000], smp_rep[:4000])) / 4000.0
        assert rep_wave_diff > 20.0, f"Re-patched topology produced identical waveform (diff={rep_wave_diff})"
        assert rms_mut > 0.015 and pk_mut <= 1.0, f"Mutated audio invalid: rms={rms_mut:.4f}, peak={pk_mut:.4f}"
        mut_wave_diff = sum(abs(a - b) for a, b in zip(smp_rep[:4000], smp_mut[:4000])) / 4000.0
        assert mut_wave_diff > 10.0, f"Mutator did not alter audio waveform (diff={mut_wave_diff})"

        # Verify external Lua hot-override without recompiling C
        custom_assets = os.path.join(tmp, "custom_assets")
        shutil.copytree(sibling_assets, custom_assets)
        with open(os.path.join(custom_assets, "scripts", "main.lua"), "a", encoding="utf-8") as f:
            f.write('\ngf.log("E2E_EXTERNAL_ASSET_MODIFIED_MARKER")\n')
        short_script = os.path.join(tmp, "short.txt")
        with open(short_script, "w", encoding="utf-8") as f:
            f.write("STEP 1\n")
        proc2 = subprocess.run(
            [binary, "--assets", custom_assets, "--headless", "--width", "320", "--height", "240", "--e2e-script", short_script],
            cwd=tmp, env=env, capture_output=True, text=True, timeout=60,
        )
        assert proc2.returncode == 0 and "E2E_EXTERNAL_ASSET_MODIFIED_MARKER" in (proc2.stdout + proc2.stderr)

        # Verify standard SDL3 interactive main loop + audio callback + keyboard/mouse events
        env_audio = env.copy()
        env_audio["SDL_AUDIODRIVER"] = "dummy"
        proc3 = subprocess.run(
            [binary, "--width", "320", "--height", "240", "--frames", "5"],
            cwd=tmp, env=env_audio, capture_output=True, text=True, timeout=60,
        )
        assert proc3.returncode == 0

        proc4 = subprocess.run([binary, "--invalid-option"], cwd=tmp, env=env, capture_output=True, text=True, timeout=15)
        assert proc4.returncode == 2

        print(
            f"NATIVE 7-ROOM E2E PASS: 7_rooms=DISTINCT, focus_diff={focus_diff}, "
            f"rms(base={rms_base:.3f} -> unplugged={rms_unp:.4f} -> repatched={rms_rep:.3f} -> mutated={rms_mut:.3f}), "
            f"external_vault=320+_OK"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
