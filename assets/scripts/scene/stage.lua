-- Green Fairy: Retro 3D Stage Choreography (Meshes, Billboarded Sprites, Optional Voxels)
local Stage = {}

Stage.time = 0.0
Stage.ids = {}

function Stage.init()
    gf.scene.clear()
    gf.scene.set_camera(0.0, 1.9, 5.6, 0.0, 0.35, 0.0, 52.0)
    gf.scene.set_retro(240.0, 24.0, true)

    Stage.ids.pedestal = gf.scene.add_mesh("models/altar_pedestal.obj")
    Stage.ids.crystal  = gf.scene.add_mesh("models/crystal_octa.obj")
    Stage.ids.ring1    = gf.scene.add_mesh("models/ring_torus.obj")
    Stage.ids.ring2    = gf.scene.add_mesh("models/ring_torus.obj")
    Stage.ids.voxel    = gf.scene.add_voxel("voxels/fairy_spire.gfvox")
    Stage.ids.wisp_l   = gf.scene.add_sprite("sprites/fairy_wisp.tga")
    Stage.ids.wisp_r   = gf.scene.add_sprite("sprites/fairy_wisp.tga")
    Stage.ids.rune     = gf.scene.add_sprite("sprites/osc_rune.tga")

    gf.scene.set_transform(Stage.ids.pedestal, 0.0, -1.25, 0.0, 0.0, 0.0, 0.0, 1.35, 1.0, 1.35)
    gf.scene.set_transform(Stage.ids.voxel,   -2.45, -0.20, -0.80, 0.0, 0.4, 0.0, 0.9, 0.9, 0.9)
    gf.scene.set_tint(Stage.ids.ring2, 0.35, 0.85, 1.0, 0.9)
    gf.scene.set_sprite_anim(Stage.ids.wisp_l, 0.72, 0.72, 4, 0)
    gf.scene.set_sprite_anim(Stage.ids.wisp_r, 0.72, 0.72, 4, 2)
    gf.scene.set_sprite_anim(Stage.ids.rune,   0.85, 0.85, 4, 0)
end

function Stage.update(dt, tel, xy_x, xy_y)
    Stage.time = Stage.time + (dt or 0.016)
    local t = Stage.time
    local rms = (tel and ((tel.rms_l or 0) + (tel.rms_r or 0)) * 0.5) or 0.0
    local pulse = 1.0 + math.min(rms * 1.4, 0.55)
    local x = xy_x or 0.5
    local y = xy_y or 0.5

    -- Orbit camera subtly with XY morph
    local cam_x = math.sin(t * 0.22) * 0.65 + (x - 0.5) * 1.4
    local cam_y = 1.75 + (0.5 - y) * 0.7
    gf.scene.set_camera(cam_x, cam_y, 5.6, 0.0, 0.35, 0.0, 52.0)

    -- Center absinthe crystal pulses & spins with audio energy
    gf.scene.set_transform(
        Stage.ids.crystal,
        0.0, 0.45 + math.sin(t * 1.8) * 0.14, 0.0,
        math.sin(t * 0.7) * 0.25, t * (0.8 + rms * 2.5), 0.0,
        pulse, pulse, pulse
    )
    gf.scene.set_tint(Stage.ids.crystal, 0.25 + x * 0.45, 0.95, 0.55 + y * 0.4, 1.0)

    -- Dual orbital oscillator rings
    gf.scene.set_transform(Stage.ids.ring1, 0.0, 0.45, 0.0, t * 0.55, t * 0.9, 0.35, 1.05, 1.05, 1.05)
    gf.scene.set_transform(Stage.ids.ring2, 0.0, 0.45, 0.0, -t * 0.4, -t * 0.7, 1.15, 0.85 * pulse, 0.85 * pulse, 0.85 * pulse)

    -- Optional voxel spire rotates slowly
    gf.scene.set_transform(Stage.ids.voxel, -2.45, -0.15 + math.sin(t * 1.1) * 0.08, -0.75, 0.0, t * 0.45, 0.0, 0.95, 0.95, 0.95)

    -- Animated camera-facing billboarded fairy sprites
    local frame = math.floor(t * 6.0) % 4
    gf.scene.set_transform(Stage.ids.wisp_l, -1.55 + math.cos(t * 1.3) * 0.3, 1.25 + math.sin(t * 2.1) * 0.22, 0.65, 0, 0, 0, 1, 1, 1)
    gf.scene.set_sprite_anim(Stage.ids.wisp_l, 0.68 * pulse, 0.68 * pulse, 4, frame)

    gf.scene.set_transform(Stage.ids.wisp_r, 1.55 + math.sin(t * 1.1) * 0.3, 1.10 + math.cos(t * 1.9) * 0.22, 0.55, 0, 0, 0, 1, 1, 1)
    gf.scene.set_sprite_anim(Stage.ids.wisp_r, 0.68 * pulse, 0.68 * pulse, 4, (frame + 2) % 4)

    gf.scene.set_transform(Stage.ids.rune, 2.35, 0.25 + math.sin(t * 1.5) * 0.15, -0.65, 0, 0, 0, 1, 1, 1)
    gf.scene.set_sprite_anim(Stage.ids.rune, 0.95, 0.95, 4, frame)
end

return Stage
