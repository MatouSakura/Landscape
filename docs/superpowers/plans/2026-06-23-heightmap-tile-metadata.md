# Heightmap Tile Metadata v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Add the first heightmap tile/package metadata layer so the current single-patch terrain can evolve toward a UE-style multi-component landscape without immediately changing rendering to multiple heightmap files.

This stage is metadata/control-layer only. It does not load multiple RAW files, stream tiles, render multiple root terrain patches, or change quadtree LOD selection.

---

## Key Changes

- Add `TerrainHeightmapTileSet` data layer:
  - `TerrainHeightmapTileSetDesc`
    - `TileCountX`
    - `TileCountZ`
    - `TileSampleCountPerAxis`
    - `Extent`
  - `TerrainHeightmapTile`
    - stable `TileIndex`
    - `TileX` / `TileZ`
    - sample origin in a future combined heightmap grid
    - `MinXZ` / `MaxXZ` / `CenterXZ`
    - per-tile sample and cell counts
  - `TerrainHeightmapTileSetStats`
    - tile counts
    - package total cell count
    - tile world size
    - layout name: `single_patch` or `tiled_grid`
  - API:
    - `Build(desc)`
    - `GetTiles()`
    - `FindTile(tileX, tileZ)`
    - `GetStats()`
    - `GetLayoutName()`

- Integrate metadata with existing terrain renderer:
  - Extend `TerrainPatchRendererDesc` with `HeightmapTileCountX` and `HeightmapTileCountZ`.
  - `TerrainPatchRenderer` builds a `TerrainHeightmapTileSet` after the heightfield source is known.
  - Default remains `1 x 1`, matching the current single fixed patch.
  - Tile metadata does not affect draw item generation yet.

- Expose diagnostics:
  - `ForwardRendererStats` adds tile/package metadata.
  - `LandscapeEditor` parses:
    - `--landscape_heightmap_tiles_x <N>`
    - `--landscape_heightmap_tiles_z <N>`
  - ImGui shows layout, tile grid, per-tile samples/cells, package cells, and tile world size.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_heightmap_tile_metadata.py
```

Expected before implementation:
- Fails because `TerrainHeightmapTileSet`, CMake entries, tile stats, CLI tile grid arguments, UI diagnostics, and validation records do not exist.

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

Smoke:
- Default D3D12, Vulkan, D3D11, and OpenGL smoke should remain visually unchanged.
- D3D12 smoke with `--landscape_heightmap_tiles_x 2 --landscape_heightmap_tiles_z 2` should run and capture successfully.
- RAW R16 D3D12 smoke with the tile metadata arguments should still render through the existing single-patch path.

Visual acceptance:
- The existing terrain, sky, grid, transparent quad, and debug overlays remain visible.
- Metadata-only tile arguments do not change geometry yet.
- UI exposes the future package layout clearly enough to verify the metadata path.

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
  - `tools\verify_landscape_component_lod_policy.py`
  - `tools\verify_landscape_lod_stitching.py`
  - `tools\verify_landscape_lod_index_stitching.py`
  - `tools\verify_landscape_lod_index_stitching_corners.py`
  - `tools\verify_landscape_external_heightmap.py`
  - `tools\verify_landscape_heightmap_tile_metadata.py`
- Smoke:
  - D3D12 default capture: `build\Win64-vs18\smoke-heightmap-tile-metadata-default-d3d12\landscape_heightmap_tile_metadata_default_d3d12.png`
  - Vulkan default capture: `build\Win64-vs18\smoke-heightmap-tile-metadata-default-vk\landscape_heightmap_tile_metadata_default_vk.png`
  - D3D11 default capture: `build\Win64-vs18\smoke-heightmap-tile-metadata-default-d3d11\landscape_heightmap_tile_metadata_default_d3d11.png`
  - OpenGL default capture: `build\Win64-vs18\smoke-heightmap-tile-metadata-default-gl\landscape_heightmap_tile_metadata_default_gl.png`
  - D3D12 `2 x 2` metadata capture: `build\Win64-vs18\smoke-heightmap-tile-metadata-grid-d3d12\landscape_heightmap_tile_metadata_grid_d3d12.png`
  - D3D12 RAW R16 `2 x 2` metadata capture: `build\Win64-vs18\smoke-heightmap-tile-metadata-raw-grid-d3d12\landscape_heightmap_tile_metadata_raw_grid_d3d12.png`
- Pixel check:
  - All captures are `640x480`, non-empty, and keep terrain/grid/sky coverage visible.
  - D3D12/Vulkan/D3D11 default captures: `non_dark=307200`, `ground_like=306560`, `grid_like=239876`, `std_sum=38.56`.
  - OpenGL default capture: `non_dark=307200`, `ground_like=306560`, `grid_like=303782`, `std_sum=48.52`.
  - Metadata-only D3D12 `2 x 2` capture is intentionally unchanged from default geometry: `grid_metadata_vs_default_changed_pixels=0`.
  - RAW R16 D3D12 `2 x 2` capture differs from default as expected: `raw_vs_default_changed_pixels=223837`.

Implementation note: this stage records and exposes the future heightmap package layout. It does not yet split rendering into multiple root components or load multiple tile files.
