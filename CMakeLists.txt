cmake_minimum_required(VERSION 3.24)
project(green_fairy VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(EMSCRIPTEN)
  set(CMAKE_C_EXTENSIONS ON)
else()
  set(CMAKE_C_EXTENSIONS OFF)
endif()

option(GF_BUILD_TESTS "Build C and Lua unit test executables" ON)
option(GF_ENABLE_COVERAGE "Enable GCC/Clang code coverage instrumentation" OFF)
option(GF_USE_SYSTEM_SDL3 "Prefer preinstalled system SDL3 via find_package" OFF)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps" CACHE PATH "Shared FetchContent cache")

# ---------------------------------------------------------------------------
# Embedded Lua 5.4.7 (static C library, identical across all platforms)
# ---------------------------------------------------------------------------
FetchContent_Declare(
  lua54
  URL https://github.com/lua/lua/archive/refs/tags/v5.4.7.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(lua54)
if(NOT lua54_POPULATED)
  FetchContent_Populate(lua54)
endif()

set(LUA54_SOURCES
  ${lua54_SOURCE_DIR}/lapi.c
  ${lua54_SOURCE_DIR}/lauxlib.c
  ${lua54_SOURCE_DIR}/lbaselib.c
  ${lua54_SOURCE_DIR}/lcode.c
  ${lua54_SOURCE_DIR}/lcorolib.c
  ${lua54_SOURCE_DIR}/lctype.c
  ${lua54_SOURCE_DIR}/ldblib.c
  ${lua54_SOURCE_DIR}/ldebug.c
  ${lua54_SOURCE_DIR}/ldo.c
  ${lua54_SOURCE_DIR}/ldump.c
  ${lua54_SOURCE_DIR}/lfunc.c
  ${lua54_SOURCE_DIR}/lgc.c
  ${lua54_SOURCE_DIR}/linit.c
  ${lua54_SOURCE_DIR}/liolib.c
  ${lua54_SOURCE_DIR}/llex.c
  ${lua54_SOURCE_DIR}/lmem.c
  ${lua54_SOURCE_DIR}/lmathlib.c
  ${lua54_SOURCE_DIR}/loadlib.c
  ${lua54_SOURCE_DIR}/lobject.c
  ${lua54_SOURCE_DIR}/lopcodes.c
  ${lua54_SOURCE_DIR}/loslib.c
  ${lua54_SOURCE_DIR}/lparser.c
  ${lua54_SOURCE_DIR}/lstate.c
  ${lua54_SOURCE_DIR}/lstring.c
  ${lua54_SOURCE_DIR}/lstrlib.c
  ${lua54_SOURCE_DIR}/ltable.c
  ${lua54_SOURCE_DIR}/ltablib.c
  ${lua54_SOURCE_DIR}/ltm.c
  ${lua54_SOURCE_DIR}/lundump.c
  ${lua54_SOURCE_DIR}/lutf8lib.c
  ${lua54_SOURCE_DIR}/lvm.c
  ${lua54_SOURCE_DIR}/lzio.c
)

add_library(lua54 STATIC ${LUA54_SOURCES})
target_include_directories(lua54 PUBLIC ${lua54_SOURCE_DIR})
if(NOT WIN32 AND NOT EMSCRIPTEN)
  target_link_libraries(lua54 PUBLIC m)
endif()

# ---------------------------------------------------------------------------
# SDL3 (pinned release 3.2.10, lean static build)
# ---------------------------------------------------------------------------
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK ON CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
if(EMSCRIPTEN)
  set(SDL_PTHREADS OFF CACHE BOOL "" FORCE)
endif()

if(GF_USE_SYSTEM_SDL3)
  find_package(SDL3 QUIET)
endif()
if(NOT TARGET SDL3::SDL3-static AND NOT TARGET SDL3::SDL3)
  FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.10.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

if(TARGET SDL3::SDL3-static)
  set(GF_SDL3_TARGET SDL3::SDL3-static)
else()
  set(GF_SDL3_TARGET SDL3::SDL3)
endif()

# ---------------------------------------------------------------------------
# Compiler warning & coverage flags (applied only to Green Fairy targets)
# ---------------------------------------------------------------------------
add_library(gf_compile_flags INTERFACE)
if(MSVC)
  target_compile_options(gf_compile_flags INTERFACE /W4)
else()
  target_compile_options(gf_compile_flags INTERFACE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if(GF_ENABLE_COVERAGE AND NOT MSVC AND NOT EMSCRIPTEN)
  target_compile_options(gf_compile_flags INTERFACE -O0 -g --coverage)
  target_link_options(gf_compile_flags INTERFACE --coverage)
endif()

# ---------------------------------------------------------------------------
# Core Engine Library (pure C11 + Lua host, testable without hardware)
# ---------------------------------------------------------------------------
set(GF_CORE_SOURCES
  src/core/math3d.c
  src/core/ringbuf.c
  src/core/prng.c
  src/core/path.c
  src/core/log.c
  src/assets/asset_manager.c
  src/assets/loaders.c
  src/assets/hot_reload.c
  src/dsp/envelope.c
  src/dsp/lfo.c
  src/dsp/oscillator.c
  src/dsp/filter.c
  src/dsp/waveshaper.c
  src/dsp/aetherizer.c
  src/dsp/fx_acoustics.c
  src/dsp/patchbay.c
  src/dsp/sequencer.c
  src/dsp/synth.c
  src/render/voxel_mesher.c
  src/render/camera_rig.c
  src/render/spatial_ui.c
  src/render/cable_renderer.c
  src/render/scene.c
  src/render/gl_renderer.c
  src/lua/lua_host.c
  src/platform/app.c
)

add_library(green_fairy_core STATIC ${GF_CORE_SOURCES})
target_include_directories(green_fairy_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(green_fairy_core PUBLIC lua54 gf_compile_flags)
if(NOT WIN32 AND NOT EMSCRIPTEN)
  target_link_libraries(green_fairy_core PUBLIC m)
endif()

# ---------------------------------------------------------------------------
# Green Fairy Standalone Executable (Desktop & Emscripten Web/WebXR)
# ---------------------------------------------------------------------------
add_executable(green_fairy
  src/main.c
  src/platform/sdl_runtime.c
)
target_link_libraries(green_fairy PRIVATE green_fairy_core ${GF_SDL3_TARGET} gf_compile_flags)

# Always place external assets beside the built executable (never baked in)
add_custom_command(TARGET green_fairy POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:green_fairy>/assets
  COMMENT "Syncing external assets/ directory beside green_fairy binary"
)

if(EMSCRIPTEN)
  set_target_properties(green_fairy PROPERTIES SUFFIX ".html")
  target_link_options(green_fairy PRIVATE
    "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
    "SHELL:-sUSE_WEBGL2=1"
    "SHELL:-sFULL_ES3=1"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sFORCE_FILESYSTEM=1"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['FS','UTF8ToString','stringToNewUTF8','HEAPF32']"
    "SHELL:-sEXPORTED_FUNCTIONS=['_main','_malloc','_free','_gf_web_start','_gf_platform_set_input_mode','_gf_web_resize','_gf_web_pointer','_gf_web_pointer_ex','_gf_web_key','_gf_web_note_on','_gf_web_note_off','_gf_web_set_xy','_gf_web_set_voxels','_gf_web_teleport_room','_gf_web_focus_rack','_gf_web_exit_focus','_gf_web_apply_preset_path','_gf_web_connect_cable','_gf_web_disconnect_port','_gf_web_mutator','_gf_web_resume_audio','_gf_webxr_begin_frame','_gf_webxr_render_eye','_gf_webxr_controller_input','_gf_webxr_controller_ray','_gf_webxr_end_frame','_gf_webxr_exit','_gf_web_get_status_json']"
  )
  add_custom_command(TARGET green_fairy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_SOURCE_DIR}/web/js
      $<TARGET_FILE_DIR:green_fairy>/js
    COMMENT "Syncing web/js/ bridge beside green_fairy.html"
  )
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(GF_BUILD_TESTS AND NOT EMSCRIPTEN)
  enable_testing()

  function(gf_add_c_test test_name test_src)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/tests/unit/c)
    target_link_libraries(${test_name} PRIVATE green_fairy_core gf_compile_flags)
    add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  endfunction()

  gf_add_c_test(test_core tests/unit/c/test_core.c)
  gf_add_c_test(test_assets tests/unit/c/test_assets.c)
  gf_add_c_test(test_patchbay tests/unit/c/test_patchbay.c)
  gf_add_c_test(test_dsp tests/unit/c/test_dsp.c)
  gf_add_c_test(test_spatial_ui tests/unit/c/test_spatial_ui.c)
  gf_add_c_test(test_render tests/unit/c/test_render.c)
  gf_add_c_test(test_lua_host tests/unit/c/test_lua_host.c)
  gf_add_c_test(test_platform tests/unit/c/test_platform.c)
  gf_add_c_test(test_preset_vault tests/unit/c/test_preset_vault.c)

  add_executable(gf_lua_test_runner tests/unit/lua/lua_test_runner.c)
  target_link_libraries(gf_lua_test_runner PRIVATE green_fairy_core gf_compile_flags)
  add_test(NAME test_lua_scripts
    COMMAND gf_lua_test_runner --assets ${CMAKE_SOURCE_DIR}/assets --fail-under 95
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()
