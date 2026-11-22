# external-assets Specification

## Purpose

Defines the external asset management pipeline ensuring all graphics, audio samples, shaders, fonts, patches, and Lua scripts live in folders beside the binary rather than being compiled into the executable.

## Requirements

### Requirement: External asset directory resolution
The engine SHALL resolve its asset root directory relative to the executable path on desktop platforms (or relative to the served web root / virtual filesystem mount on Emscripten), with an optional command-line or environment override (`GREEN_FAIRY_ASSETS`).

#### Scenario: Discovering sibling assets folder next to binary
- **GIVEN** an executable located at `<bin_dir>/green_fairy` with an `assets/` folder beside it at `<bin_dir>/assets/`
- **WHEN** the engine starts without an explicit override
- **THEN** it resolves `<bin_dir>/assets/` as the authoritative asset root and loads subfolders for `scripts/`, `patches/`, `models/`, `sprites/`, `voxels/`, `shaders/`, `sounds/`, and `fonts/`

#### Scenario: Explicit asset directory override
- **GIVEN** a custom external asset directory passed via `--assets <dir>` or `GREEN_FAIRY_ASSETS`
- **WHEN** the engine starts
- **THEN** all assets are loaded exclusively from the specified override directory

### Requirement: Zero embedded binary assets
The compiled executable SHALL NOT embed 3D models, sprite textures, voxel volumes, shader source strings, audio samples, patches, or Lua scripts inside binary data arrays; modifying files in the external asset folders MUST take effect without recompiling the executable.

#### Scenario: User replaces an external asset file without recompiling
- **GIVEN** a compiled Green Fairy binary and an external sprite texture or Lua patch file in the sibling `assets/` directory
- **WHEN** the external file is replaced with a modified valid file and reloaded or restarted
- **THEN** the application displays or plays the modified asset without rebuilding the binary

### Requirement: Typed external asset loaders and validation
The asset subsystem SHALL load and validate external files for 3D meshes (OBJ/JSON), sprites (PNG/TGA/PPM), optional voxel volumes (simple JSON/VOX format), GLSL shaders, bitmap/TTF fonts, PCM WAV audio samples, and Lua scripts, rejecting malformed inputs safely.

#### Scenario: Handling missing or malformed asset files
- **GIVEN** a reference to a missing or corrupted external mesh, sprite, voxel, or WAV file
- **WHEN** the asset loader attempts to load the file
- **THEN** the loader returns a structured error, logs the offending path, and prevents memory corruption or application crash

### Requirement: Path traversal protection
The asset subsystem SHALL normalize requested relative asset paths and reject any path attempting to escape the configured external asset root directory.

#### Scenario: Rejecting directory escape path
- **GIVEN** a Lua script or patch requesting an asset path containing `../` segments outside the asset root
- **WHEN** the asset resolver evaluates the path
- **THEN** the request is rejected with a permission/path error and no file outside the asset root is opened

### Requirement: Runtime asset hot-reload
The asset subsystem SHALL detect file modification changes (or respond to an explicit reload trigger) for external scripts, shaders, patches, and graphics assets and reload them safely on the main thread.

#### Scenario: Reloading a modified shader or script at runtime
- **GIVEN** a running Green Fairy instance with an active shader or Lua script loaded from `assets/`
- **WHEN** the file is edited on disk and hot-reload runs
- **THEN** the updated shader or script replaces the active version if valid, or retains the previous version with a logged diagnostic if invalid
