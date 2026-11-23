-- Seven-Room Retro-3D Synth Sanctum Architectural Floorplan & Audio Choreography

local Floorplan = {
    wisps = {},
    spires = {},
    shells = {},
    arches = {},
    pedestal = -1,
    time = 0.0,
}

local CORRIDOR_LINKS = {
    {6, 1}, {6, 2}, {6, 3}, {6, 4}, {6, 5}, {6, 7},
    {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 7}, {7, 1},
}

function Floorplan.build_architecture()
    Floorplan.wisps = {}
    Floorplan.spires = {}
    Floorplan.shells = {}
    Floorplan.arches = {}

    local rooms = gf.spatial.rooms()
    for _, rm in ipairs(rooms) do
        local th = rm.theme
        -- Vaulted octagonal room shell
        local shell = gf.scene.add_mesh("models/room_shell.obj", rm.id)
        gf.scene.set_transform(shell, rm.x, 0.0, rm.z, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0)
        gf.scene.set_tint(shell, 0.45 + th[1] * 0.55, 0.45 + th[2] * 0.55, 0.45 + th[3] * 0.55, 1.0)
        Floorplan.shells[#Floorplan.shells + 1] = { id = shell, theme = th }

        -- Optional decorative voxel sanctum spires flanking the rear archway
        local sp = gf.scene.add_voxel("voxels/fairy_spire.gfvox", rm.id)
        gf.scene.set_transform(sp, rm.x - 4.2, 0.05, rm.z - 4.2, 0.0, 0.4, 0.0, 0.9, 0.9, 0.9)
        gf.scene.set_tint(sp, th[1], th[2], th[3], 1.0)
        Floorplan.spires[#Floorplan.spires + 1] = sp

        -- Audio-reactive floating Green Fairy light wisp per room crown
        local w = gf.scene.add_sprite("sprites/fairy_wisp.tga", rm.id)
        gf.scene.set_transform(w, rm.x, 3.1, rm.z, 0, 0, 0, 1, 1, 1)
        gf.scene.set_sprite_anim(w, 0.72, 0.72, 4, 0)
        gf.scene.set_tint(w, th[1], th[2], th[3], 0.92)
        Floorplan.wisps[#Floorplan.wisps + 1] = { id = w, cx = rm.x, cz = rm.z, phase = rm.id * 0.9 }
    end

    -- 12 Inter-room corridor bridges & overhead cable conduit arches
    for _, link in ipairs(CORRIDOR_LINKS) do
        local ra = rooms[link[1]]
        local rb = rooms[link[2]]
        local mx = (ra.x + rb.x) * 0.5
        local mz = (ra.z + rb.z) * 0.5
        local dx = rb.x - ra.x
        local dz = rb.z - ra.z
        local dist = math.sqrt(dx * dx + dz * dz)
        local yaw = math.atan(dx, dz)
        local arch = gf.scene.add_mesh("models/archway_conduit.obj", 0)
        gf.scene.set_transform(arch, mx, 0.0, mz, 0.0, yaw, 0.0, 1.0, 1.0, math.max(1.0, (dist - 10.5) / 4.0))
        gf.scene.set_tint(arch, 0.55, 0.92, 0.82, 1.0)
        Floorplan.arches[#Floorplan.arches + 1] = arch
    end

    -- Central Room 6 Holographic XYZ Morph Pedestal
    Floorplan.pedestal = gf.scene.add_mesh("models/morph_pedestal.obj", 6)
    gf.scene.set_transform(Floorplan.pedestal, 0.0, 0.0, -1.35, 0.0, 0.0, 0.0, 1.15, 1.05, 1.15)
    gf.scene.set_tint(Floorplan.pedestal, 0.45, 1.0, 0.84, 1.0)
end

function Floorplan.apply_world_tint(r, g, b)
    -- Keep foreground sanctum architecture in sleek dark basalt/gunmetal slate
    -- with only a refined ~10% ambient bounce reflection so it stays coloristically
    -- separated from the vibrant distant surrounding world.
    for _, sh in ipairs(Floorplan.shells) do
        local th = sh.theme
        gf.scene.set_tint(sh.id,
            0.22 + th[1] * 0.07 + r * 0.09,
            0.24 + th[2] * 0.07 + g * 0.09,
            0.28 + th[3] * 0.07 + b * 0.09, 1.0)
    end
    for _, arch in ipairs(Floorplan.arches) do
        gf.scene.set_tint(arch, 0.30 + r * 0.12, 0.32 + g * 0.12, 0.36 + b * 0.12, 1.0)
    end
    for _, sp in ipairs(Floorplan.spires) do
        gf.scene.set_tint(sp, 0.28 + r * 0.26, 0.30 + g * 0.26, 0.34 + b * 0.26, 1.0)
    end
    if Floorplan.pedestal and Floorplan.pedestal > 0 then
        gf.scene.set_tint(Floorplan.pedestal, 0.32 + r * 0.24, 0.35 + g * 0.24, 0.38 + b * 0.24, 1.0)
    end
end

function Floorplan.update(dt, tel)
    Floorplan.time = Floorplan.time + dt
    local t = Floorplan.time
    local energy = tel and ((tel.rms_l or 0) + (tel.rms_r or 0)) * 0.5 or 0.0
    local frame = math.floor(t * 9.0) % 4

    for _, w in ipairs(Floorplan.wisps) do
        local ang = t * 0.75 + w.phase
        local wx = w.cx + math.cos(ang) * 2.1
        local wz = w.cz + math.sin(ang) * 2.1
        local wy = 3.05 + math.sin(ang * 2.0) * 0.22 + energy * 0.45
        gf.scene.set_transform(w.id, wx, wy, wz, 0, 0, 0, 1.0 + energy * 0.6, 1.0 + energy * 0.6, 1.0)
        gf.scene.set_sprite_anim(w.id, 0.70, 0.70, 4, frame)
    end
end

return Floorplan
