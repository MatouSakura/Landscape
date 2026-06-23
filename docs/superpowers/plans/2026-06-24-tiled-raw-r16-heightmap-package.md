# Tiled RAW R16 Heightmap Package v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Load a square grid of RAW R16 heightmap tiles into the existing `TerrainHeightField` so the current quadtree/LOD/stitching renderer can consume a combined heightfield package.

This stage is CPU data-layer work. It does not stream tiles, render multiple root components, add PNG/R32F decoding, or introduce per-component material/layer data.

---

## Design

- Add a tiled RAW R16 loader to `TerrainHeightField`:
  - New source enum value: `RawR16Tiles`.
  - New API: `LoadRawR16Tiles(desc, pattern, tileCountX, tileCountZ, tileSampleCountPerAxis, errorMessage)`.
  - The path pattern uses `{x}` and `{z}` tokens, for example:
    `E:\Landscape\build\Win64-vs18\test-heightmaps\tile_{x}_{z}.r16`
  - Every tile is square, little-endian RAW R16, and has the same sample count.
  - Tile edge samples overlap. A `2 x 2` package with `33 x 33` samples per tile produces a combined `65 x 65` heightfield.
  - V1 requires a square tile grid for actual loading because `TerrainHeightField` currently stores one scalar `CellCount`.

- Integrate with existing renderer:
  - Extend `TerrainPatchRendererDesc` with `HeightmapRawR16TilesPattern`.
  - Tiled RAW R16 has priority over single RAW R16 if the tiled pattern is provided.
  - Existing procedural fallback remains unchanged.
  - `TerrainPatchRenderer` still builds one packed mesh cache over the combined heightfield.
  - Existing quadtree selection, LOD tile sampling, skirts, stitching, shadows, and forward rendering are reused.

- Expose command-line control:
  - `--landscape_heightmap_raw_r16_tiles <pattern>`
  - Existing `--landscape_heightmap_tiles_x <N>` and `--landscape_heightmap_tiles_z <N>` define package dimensions.
  - Existing `--landscape_heightmap_samples <N>` becomes the per-tile sample count for tiled RAW R16.
  - Existing `--landscape_heightmap_height_scale <S>` still controls vertical scale.

## Key Changes

- Modify `TerrainHeightField`:
  - Add `RawR16Tiles` source name as `raw_r16_tiles`.
  - Add shared RAW R16 file reading helper.
  - Add token expansion for `{x}` and `{z}`.
  - Merge tile samples into a combined height grid before stats/normals rebuild.

- Modify `TerrainPatchRenderer`:
  - Accept tiled RAW R16 path pattern.
  - Prefer tiled package load over single-file load.
  - Build heightmap tile metadata with per-tile sample count when a tiled package is requested.

- Modify `ForwardRenderer` and `LandscapeEditor`:
  - Add setter and CLI wiring for tiled RAW R16 package pattern.
  - Reject non-square tiled load grids in V1 with a clear command-line error.
  - Keep metadata-only non-square grids available when no tiled RAW R16 package is loaded.

- Update validation and docs:
  - Add `tools/verify_landscape_tiled_raw_r16_heightmap.py`.
  - Update `PROJECT_STATUS.md` with done/next/bugs.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_tiled_raw_r16_heightmap.py
```

Expected before implementation:
- Fails because `LoadRawR16Tiles`, `raw_r16_tiles`, CLI pattern parsing, tiled package docs, and validation records do not exist.

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
- `tools\verify_landscape_heightmap_tile_metadata.py`
- `tools\verify_landscape_tiled_raw_r16_heightmap.py`

Smoke:
- Default D3D12/Vulkan/D3D11/OpenGL smoke should remain visible.
- D3D12/Vulkan tiled RAW R16 smoke should load a `2 x 2` package of `33 x 33` tiles.
- D3D12 invalid non-square tiled package command should exit with an error.

Visual acceptance:
- Tiled RAW R16 capture differs from procedural and single RAW R16 captures.
- Terrain, sky, grid, quadtree overlay, shadows, transparent quad, and stitched LOD draw path remain visible.
- `Height source` reports `raw_r16_tiles`.

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
- Generated validation asset:
  - `build\Win64-vs18\test-heightmaps\tiled-raw-r16-2x2\tile_{x}_{z}.r16`
  - Four `33 x 33` little-endian RAW R16 files, `2178` bytes each.
- Smoke:
  - D3D12 default capture: `build\Win64-vs18\smoke-tiled-raw-r16-default-d3d12\landscape_tiled_raw_r16_default_d3d12.png`
  - Vulkan default capture: `build\Win64-vs18\smoke-tiled-raw-r16-default-vk\landscape_tiled_raw_r16_default_vk.png`
  - D3D11 default capture: `build\Win64-vs18\smoke-tiled-raw-r16-default-d3d11\landscape_tiled_raw_r16_default_d3d11.png`
  - OpenGL default capture: `build\Win64-vs18\smoke-tiled-raw-r16-default-gl\landscape_tiled_raw_r16_default_gl.png`
  - D3D12 tiled package capture: `build\Win64-vs18\smoke-tiled-raw-r16-package-d3d12\landscape_tiled_raw_r16_package_d3d12.png`
  - Vulkan tiled package capture: `build\Win64-vs18\smoke-tiled-raw-r16-package-vk\landscape_tiled_raw_r16_package_vk.png`
  - D3D12 non-square tiled package command exited with `-1`, as expected.
- Pixel check:
  - All captures are `640x480`, non-empty, and keep terrain/grid/sky coverage visible.
  - Default D3D12/Vulkan/D3D11 captures: `non_dark=307200`, `ground_like=307200`, `grid_like=239876`, `std_sum=38.56`.
  - Default OpenGL capture: `non_dark=307200`, `ground_like=307200`, `grid_like=303782`, `std_sum=48.52`.
  - Tiled D3D12/Vulkan captures: `non_dark=307200`, `ground_like=307200`, `grid_like=261748`, `std_sum=41.43`.
  - Tiled-vs-default changed pixels: D3D12 `157443`, Vulkan `157443`.

Implementation note: v1 intentionally keeps the renderer on one combined CPU heightfield. The next structural step is either non-square heightfield storage or true multi-root terrain components.
