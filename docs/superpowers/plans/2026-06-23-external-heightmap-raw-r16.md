# External Heightmap RAW R16 v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Add the first external heightmap data path for one fixed terrain patch while preserving the current procedural heightfield fallback and all quadtree/LOD/stitching behavior.

This stage supports a square little-endian RAW R16 file loaded on CPU. It does not add PNG loading, R32F loading, tiled packages, streaming, import UI, or heightmap editing.

---

## Key Changes

- Extend `TerrainHeightField`:
  - Add `TerrainHeightFieldSource { Procedural, RawR16 }`.
  - Add `LoadRawR16(desc, path, sampleCountPerAxis, errorMessage)`.
  - Interpret unsigned R16 samples as normalized heights in `[-HeightScale, HeightScale]`.
  - Reuse the existing CPU stats and central-difference normal generation for both procedural and RAW R16 sources.
- Extend terrain renderer configuration:
  - Add `TerrainPatchRendererDesc`.
  - Pass an optional `HeightmapRawR16Path`, sample count, and height scale before terrain mesh cache creation.
  - Default to procedural generation when no path is supplied.
  - Log and fall back to procedural generation if the RAW R16 file is invalid.
- Extend `ForwardRenderer` / `LandscapeEditor`:
  - Add command-line arguments:
    - `--landscape_heightmap_raw_r16 <path>`
    - `--landscape_heightmap_samples <N>`
    - `--landscape_heightmap_height_scale <scale>`
  - Add ImGui source diagnostics: procedural vs raw_r16 and loaded state.
  - Keep existing camera presets, LOD policy, skirts, stitching, shadows, and smoke paths unchanged.
- Update validation:
  - Add `tools/verify_landscape_external_heightmap.py`.
  - Add a generated RAW R16 smoke asset under the build directory during validation, not as a tracked binary.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_external_heightmap.py
```

Expected before implementation:
- Fails because RAW R16 API, command-line arguments, renderer descriptor, height source stats, UI source display, and validation record do not exist.

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
- `tools\verify_landscape_component_lod_policy.py`
- `tools\verify_landscape_lod_stitching.py`
- `tools\verify_landscape_lod_index_stitching.py`
- `tools\verify_landscape_lod_index_stitching_corners.py`
- `tools\verify_landscape_external_heightmap.py`

Smoke:
- Default procedural smoke remains valid on D3D12, Vulkan, D3D11, and OpenGL.
- RAW R16 smoke runs on D3D12 and Vulkan with:
  - `--landscape_heightmap_raw_r16 <generated raw path>`
  - `--landscape_heightmap_samples 65`
  - `--landscape_heightmap_height_scale 4.0`

Visual acceptance:
- Procedural default still renders terrain/grid/sky/transparent/debug overlays.
- RAW R16 path produces a different visible terrain silhouette and non-flat height range.
- LOD stitching and quadtree overlays remain visible with the loaded heightmap.

## Validation Record

Completed on 2026-06-23.

Build:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build E:\Landscape\build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Static validation:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_external_heightmap.py
```

Generated RAW R16 validation asset:

- `build\Win64-vs18\test-heightmaps\ridge_65x65.r16`
- Size: `8450` bytes (`65 * 65 * 2`).

Smoke:

- Default procedural D3D12: `build\Win64-vs18\smoke-external-heightmap-default-d3d12\landscape_external_heightmap_default_d3d12.png`
- Default procedural Vulkan: `build\Win64-vs18\smoke-external-heightmap-default-vk\landscape_external_heightmap_default_vk.png`
- Default procedural D3D11: `build\Win64-vs18\smoke-external-heightmap-default-d3d11\landscape_external_heightmap_default_d3d11.png`
- Default procedural OpenGL: `build\Win64-vs18\smoke-external-heightmap-default-gl\landscape_external_heightmap_default_gl.png`
- RAW R16 D3D12: `build\Win64-vs18\smoke-external-heightmap-raw-d3d12\landscape_external_heightmap_raw_d3d12.png`
- RAW R16 Vulkan: `build\Win64-vs18\smoke-external-heightmap-raw-vk\landscape_external_heightmap_raw_vk.png`
- RAW R16 D3D11: `build\Win64-vs18\smoke-external-heightmap-raw-d3d11\landscape_external_heightmap_raw_d3d11.png`
- RAW R16 OpenGL: `build\Win64-vs18\smoke-external-heightmap-raw-gl\landscape_external_heightmap_raw_gl.png`

Pixel check:

- Default D3D12/Vulkan/D3D11: `640x480`, `non_dark=307200`, `ground_like=237188`, `grid_like=252784`, `std_sum=38.56`.
- Default OpenGL: `640x480`, `non_dark=307200`, `ground_like=304175`, `grid_like=293451`, `std_sum=48.52`.
- RAW D3D12/Vulkan/D3D11: `640x480`, `non_dark=307200`, `ground_like=265860`, `grid_like=272432`, `std_sum=33.05`.
- RAW OpenGL: `640x480`, `non_dark=307200`, `ground_like=304175`, `grid_like=303782`, `std_sum=45.32`.
- RAW-vs-procedural changed pixels: D3D12 `114294`, Vulkan `114294`, D3D11 `114294`, OpenGL `108922`.

Result:

- RAW R16 loading, command-line configuration, source stats, and procedural fallback are implemented.
- Default procedural terrain remains compatible with existing four-backend smoke.
- RAW R16 terrain uses the existing quadtree LOD, packed tile mesh cache, skirts, stitching, shadows, and forward render path.
