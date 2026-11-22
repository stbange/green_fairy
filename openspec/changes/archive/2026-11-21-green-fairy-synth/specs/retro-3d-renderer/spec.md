# Spec Delta

## Purpose

Defines the retro 3D OpenGL and WebGL2 rendering pipeline supporting 3D meshes, billboarded sprites, optional voxel structures, audio-reactive visualizers, retro post-processing, hybrid HUD rendering, and stereoscopic WebXR views.

## ADDED Requirements

### Requirement: Retro 3D mesh rendering
The renderer SHALL load and draw indexed 3D triangle meshes with vertex positions, normals, UVs, and vertex colors using perspective projection, depth testing, and retro aesthetic shading controls (vertex lighting, optional low-precision vertex snapping, and color quantization).

#### Scenario: Rendering an external 3D object
- **GIVEN** an external 3D mesh loaded into the scene graph with a non-zero transform and active camera
- **WHEN** a frame is rendered to the color framebuffer
- **THEN** pixels covered by the projected mesh geometry differ from the clear background color and respect depth ordering

### Requirement: 3D billboarded sprite rendering
The renderer SHALL draw textured 2D sprites positioned in 3D world space with camera-facing billboarding, alpha testing or alpha blending, and sprite-sheet frame animation.

#### Scenario: Camera-facing sprite animation
- **GIVEN** a sprite entity in 3D space referencing an external sprite texture atlas with multiple animation frames
- **WHEN** the camera orbits the sprite and animation time advances
- **THEN** the sprite quad faces the camera and samples the active frame coordinates without visual artifacts

### Requirement: Optional voxel grid rendering
The renderer SHALL support toggling optional voxel structures built from external voxel volume data, converting occupied voxels into culled surface geometry for efficient rendering.

#### Scenario: Enabling and disabling voxel structures
- **GIVEN** a scene containing an optional voxel entity loaded from an external voxel file
- **WHEN** the voxel rendering option is toggled off and back on
- **THEN** the voxel geometry is excluded from draw calls when disabled and rendered with culled hidden internal faces when enabled

### Requirement: Audio-reactive 3D visualization
The renderer SHALL consume lock-free audio analysis snapshots (real-time waveform ring buffer, spectral/RMS energy bands, active voice count, and envelope levels) to drive 3D geometry transforms, ribbons, sprites, and shader uniforms.

#### Scenario: Audio energy drives visual state
- **GIVEN** active note playback producing non-zero RMS and spectral energy
- **WHEN** the audio snapshot is published to the renderer for the next frame
- **THEN** audio-reactive visual uniforms and visualizer ribbons reflect the non-zero waveform and energy levels

### Requirement: Hybrid Lua-driven 2D HUD and spatial WebXR panels
The renderer SHALL render a Lua-defined 2D overlay HUD for desktop and mobile touch viewports, and SHALL project interactive UI panels onto spatial quads inside 3D space when running an immersive WebXR session.

#### Scenario: Rendering HUD controls over the 3D scene
- **GIVEN** a Lua UI script declaring parameter knobs, an XY morph pad, and a virtual keyboard
- **WHEN** a frame is rendered in desktop or mobile viewport mode
- **THEN** the 3D scene renders first and the 2D HUD composites cleanly on top with crisp hit-test alignment

#### Scenario: Stereoscopic WebXR frame rendering
- **GIVEN** an active WebXR immersive session providing left-eye and right-eye view and projection matrices
- **WHEN** an XR frame callback executes
- **THEN** the renderer draws both eye viewports with their respective view/projection matrices including spatial UI panels

### Requirement: Cross-API shader compatibility
The renderer SHALL execute cleanly on both desktop OpenGL 3.3 Core and WebGL2 / OpenGL ES 3.0 using external shader assets without compilation errors.

#### Scenario: External shader compilation
- **GIVEN** external vertex and fragment shader files loaded from the shaders asset directory
- **WHEN** the renderer initializes on desktop OpenGL or WebGL2
- **THEN** all required shader programs compile and link cleanly with explicit diagnostic logging on syntax failure
