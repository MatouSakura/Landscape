# Non-square Heightfield Storage v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Remove the square-heightfield storage restriction so tiled RAW R16 packages such as `2 x 1` and `3 x 2` can load into one combined CPU heightfield.

This stage keeps the current square quadtree world extent and single combined terrain draw path. It does not add true multi-root components, per-component culling roots, streaming, or non-square world extents.

---

## Design

- Extend `TerrainHeightField` storage:
  - Keep legacy `CellCount` and `SampleCountPerAxis` accessors for existing callers and verification scripts.
  - Add X/Z-specific cell/sample accessors:
    - `GetCellCountX()`
    - `GetCellCountZ()`
    - `GetSampleCountX()`
    - `GetSampleCountZ()`
    - `GetCellSizeX()`
    - `GetCellSizeZ()`
  - Internally index rows with `m_SampleCountX`, not the old square sample count.
  - Generate UVs from independent X/Z sample dimensions.
  - Rebuild normals using independent X/Z cell sizes.

- Update RAW R16 loading:
  - Single-file RAW R16 remains square.
  - Tiled RAW R16 no longer requires `TileCountX == TileCountZ`.
  - A `2 x 1` package with `33 x 33` tiles becomes `65 x 33`.
  - A `3 x 2` package with `33 x 33` tiles becomes `97 x 65`.

- Update terrain mesh generation:
  - `BuildTerrainDrawRegion()` maps X and Z independently using `GetCellCountX()` and `GetCellCountZ()`.
  - Packed tile mesh generation uses `GetCellSizeX()` and `GetCellSizeZ()` for world positions.
  - Existing quadtree, LOD sampling, skirts, stitching, shadow, and forward rendering paths remain in place.

- Update diagnostics:
  - `ForwardRendererStats` exposes X/Z terrain cell and sample counts.
  - ImGui shows `Terrain cells: X x Z` and `Terrain samples: X x Z`.
  - Existing scalar diagnostics remain for compatibility.

## Key Changes

- Modify `TerrainHeightField`:
  - Add rectangular storage members.
  - Keep procedural and single RAW R16 behavior unchanged by setting X/Z dimensions equal.
  - Remove square-grid rejection from `LoadRawR16Tiles()`.

- Modify `TerrainPatchRenderer`:
  - Use X/Z heightfield dimensions for draw region mapping and vertex placement.
  - Add accessors for X/Z cell/sample counts.

- Modify `ForwardRenderer` and `LandscapeEditor`:
  - Add X/Z stats.
  - Remove CLI non-square tiled package rejection.

- Update validation and docs:
  - Add `tools/verify_landscape_non_square_heightfield.py`.
  - Update `PROJECT_STATUS.md`.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_non_square_heightfield.py
```

Expected before implementation:
- Fails because heightfield storage still uses one sample axis, tiled RAW R16 still rejects non-square tile grids, and renderer stats/UI do not expose X/Z dimensions.

Build:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build E:\Landscape\build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Static validation:
- All existing `tools\verify_landscape_*.py` scripts.
- New `tools\verify_landscape_non_square_heightfield.py`.

Smoke:
- Default D3D12/Vulkan/D3D11/OpenGL captures remain visible.
- D3D12 and Vulkan smoke a `2 x 1` tiled RAW R16 package with `33 x 33` tile samples.
- The previous non-square error-path smoke is removed because non-square tiled packages are now accepted.

Visual acceptance:
- Non-square tiled RAW R16 capture differs from default procedural terrain.
- Terrain/grid/sky/quadtree overlay/shadows remain visible.
- `Height source` reports `raw_r16_tiles`.
- UI reports distinct X/Z sample counts for `2 x 1`.

## Validation Record

Completed on 2026-06-24.

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
  - `tools\verify_landscape_component_lod_policy.py`
  - `tools\verify_landscape_lod_stitching.py`
  - `tools\verify_landscape_lod_index_stitching.py`
  - `tools\verify_landscape_lod_index_stitching_corners.py`
  - `tools\verify_landscape_external_heightmap.py`
  - `tools\verify_landscape_heightmap_tile_metadata.py`
  - `tools\verify_landscape_tiled_raw_r16_heightmap.py`
  - `tools\verify_landscape_non_square_heightfield.py`
- Generated validation asset:
  - `build\Win64-vs18\test-heightmaps\tiled-raw-r16-2x1\tile_{x}_{z}.r16`
  - Two `33 x 33` little-endian RAW R16 files, `2178` bytes each.
  - Combined heightfield sample grid: `65 x 33`.
- Smoke:
  - D3D12 default capture: `build\Win64-vs18\smoke-non-square-heightfield-default-d3d12\landscape_non_square_heightfield_default_d3d12.png`
  - Vulkan default capture: `build\Win64-vs18\smoke-non-square-heightfield-default-vk\landscape_non_square_heightfield_default_vk.png`
  - D3D11 default capture: `build\Win64-vs18\smoke-non-square-heightfield-default-d3d11\landscape_non_square_heightfield_default_d3d11.png`
  - OpenGL default capture: `build\Win64-vs18\smoke-non-square-heightfield-default-gl\landscape_non_square_heightfield_default_gl.png`
  - D3D12 `2 x 1` tiled RAW R16 capture: `build\Win64-vs18\smoke-non-square-heightfield-2x1-d3d12\landscape_non_square_heightfield_2x1_d3d12.png`
  - Vulkan `2 x 1` tiled RAW R16 capture: `build\Win64-vs18\smoke-non-square-heightfield-2x1-vk\landscape_non_square_heightfield_2x1_vk.png`
- Pixel check:
  - All captures are `640x480`, non-empty, and keep terrain/grid/sky coverage visible.
  - Default D3D12/Vulkan/D3D11 captures: `non_dark=307200`, `ground_like=307200`, `grid_like=239876`, `std_sum=38.56`.
  - Default OpenGL capture: `non_dark=307200`, `ground_like=307200`, `grid_like=303782`, `std_sum=48.52`.
  - Non-square D3D12/Vulkan captures: `non_dark=307200`, `ground_like=307200`, `grid_like=255648`, `std_sum=34.61`.
  - Non-square-vs-default changed pixels: D3D12 `206188`, Vulkan `206188`.

Implementation note: this removes the square package restriction for CPU heightfield storage. It deliberately leaves true multi-root terrain components and streaming for later.
