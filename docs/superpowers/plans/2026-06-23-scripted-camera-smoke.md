# Scripted Camera Smoke Presets Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Add deterministic camera presets to `LandscapeEditor` so smoke captures can reliably exercise the default view, mixed-LOD transition areas, and off-frustum terrain culling views.

**Architecture:** Keep the runtime camera interactive by default. Add a Landscape-specific command-line argument handled by `LandscapeEditor::ProcessCommandLine()` after `SampleApp` consumes common arguments:

```powershell
--landscape_camera_preset default
--landscape_camera_preset mixed_lod
--landscape_camera_preset off_frustum
```

The editor maps the preset to a camera position and yaw/pitch during initialization, before `UpdateRenderView()` builds matrices. The fixed presets are only smoke/regression aids; they do not replace runtime input and do not change terrain LOD or culling logic.

---

## Key Changes

- Extend `LandscapeEditor`:
  - Add `LandscapeCameraPreset`.
  - Add command-line parsing for `--landscape_camera_preset`.
  - Apply preset camera position and rotation during `Initialize()`.
  - Show the active preset in ImGui.
- Add validation:
  - `tools/verify_landscape_scripted_camera_smoke.py`.
- Add smoke captures:
  - Default four-backend smoke remains the regular regression set.
  - Additional D3D12 scripted captures for `mixed_lod` and `off_frustum`.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_scripted_camera_smoke.py
```

Expected before implementation:
- Fails because the command-line enum, preset application, UI field, status record, and validation record are not present.

Build:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build E:\Landscape\build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Static validation:
- `tools\verify_landscape_stage4.py`
- `tools\verify_landscape_stage5.py`
- `tools\verify_landscape_stage6.py`
- `tools\verify_landscape_forward_completion.py`
- `tools\verify_landscape_heightfield.py`
- `tools\verify_landscape_quadtree_lod.py`
- `tools\verify_landscape_selected_leaf_render_items.py`
- `tools\verify_landscape_packed_tile_mesh_cache.py`
- `tools\verify_landscape_tile_skirts.py`
- `tools\verify_landscape_lod_tile_sampling.py`
- `tools\verify_landscape_lod_transition_diagnostics.py`
- `tools\verify_landscape_frustum_culling.py`
- `tools\verify_landscape_scripted_camera_smoke.py`

Smoke:
- D3D12, Vulkan, D3D11, OpenGL default captures remain valid.
- D3D12 scripted captures into:
  - `build\Win64-vs18\smoke-scripted-camera-mixed-lod-d3d12`
  - `build\Win64-vs18\smoke-scripted-camera-off-frustum-d3d12`

Visual acceptance:
- Default preset keeps existing visual baseline.
- `mixed_lod` captures terrain, sky, grid, transparent quad, and debug overlays from a mixed-distance camera angle.
- `off_frustum` captures a side-biased view where frustum culling can remove part of the selected leaf set without blanking the frame.
- No PSO creation happens during render.

## Validation Record

Completed on 2026-06-23.

- Build: `LandscapeEditor` Release target succeeded with VS18 CMake.
- Static validation:
  - `tools\verify_landscape_stage4.py`
  - `tools\verify_landscape_stage5.py`
  - `tools\verify_landscape_stage6.py`
  - `tools\verify_landscape_forward_completion.py`
  - `tools\verify_landscape_heightfield.py`
  - `tools\verify_landscape_quadtree_lod.py`
  - `tools\verify_landscape_selected_leaf_render_items.py`
  - `tools\verify_landscape_packed_tile_mesh_cache.py`
  - `tools\verify_landscape_tile_skirts.py`
  - `tools\verify_landscape_lod_tile_sampling.py`
  - `tools\verify_landscape_lod_transition_diagnostics.py`
  - `tools\verify_landscape_frustum_culling.py`
  - `tools\verify_landscape_scripted_camera_smoke.py`
- Smoke:
  - D3D12 default: `build\Win64-vs18\smoke-scripted-camera-default-d3d12\landscape_scripted_default_d3d12.png`
  - Vulkan default: `build\Win64-vs18\smoke-scripted-camera-default-vk\landscape_scripted_default_vk.png`
  - D3D11 default: `build\Win64-vs18\smoke-scripted-camera-default-d3d11\landscape_scripted_default_d3d11.png`
  - OpenGL default: `build\Win64-vs18\smoke-scripted-camera-default-gl\landscape_scripted_default_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-scripted-camera-mixed-lod-d3d12\landscape_scripted_mixed_lod_d3d12.png`
  - D3D12 `off_frustum`: `build\Win64-vs18\smoke-scripted-camera-off-frustum-d3d12\landscape_scripted_off_frustum_d3d12.png`
- Pixel check:
  - D3D12 default: `640x480`, `646` unique colors, `186081` terrain-like pixels, `2618` cyan diagnostic pixels.
  - Vulkan default: `640x480`, `646` unique colors, `186081` terrain-like pixels, `2618` cyan diagnostic pixels.
  - D3D11 default: `640x480`, `646` unique colors, `186081` terrain-like pixels, `2618` cyan diagnostic pixels.
  - OpenGL default: `640x480`, `1876` unique colors, `160977` terrain-like pixels, `7838` cyan diagnostic pixels.
  - D3D12 `mixed_lod`: `640x480`, `348` unique colors, `43870` terrain-like pixels, `697` cyan diagnostic pixels.
  - D3D12 `off_frustum`: `640x480`, `360` unique colors, `31163` terrain-like pixels, `635` cyan diagnostic pixels.
- Visual check:
  - `mixed_lod` shows terrain from a side-biased camera and keeps skirt/leaf diagnostics visible.
  - `off_frustum` keeps a partial terrain view visible while biasing the camera toward culling-regression coverage.
- Implementation note: `LandscapeEditor::ProcessCommandLine()` consumes `--landscape_camera_preset` after `SampleApp` removes common args. The preset is applied during initialization before `UpdateRenderView()`, and invalid preset names return `CommandLineStatus::Error`.
