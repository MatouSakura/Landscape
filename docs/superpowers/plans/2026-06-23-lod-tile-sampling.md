# LOD Tile Sampling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep each implementation stage documented, built, validated, committed, and pushed.

**Goal:** Make selected quadtree leaves render with real level-dependent mesh density instead of drawing every selected leaf at full heightfield resolution.

**Architecture:** `TerrainPatchRenderer` continues to prebuild one packed mesh range for every quadtree node. During packed mesh cache generation it computes a deterministic LOD sample step from the node level and max quadtree level:

```text
LODSampleStep = 1 << (MaxQuadtreeLevel - Node.Level)
```

Level 4 tiles use step `1`, level 3 uses `2`, level 2 uses `4`, level 1 uses `8`, and level 0 uses `16` for the default `64x64` heightfield and `MaxDepth=4`. Each tile still includes its exact min/max cell boundaries, so the mesh covers the same world-space bounds while using fewer interior vertices at coarser LOD levels.

This stage does not add geomorphing, neighbor-aware stitching, or screen-space error metrics. The existing tile skirts remain the first-pass crack-hiding policy.

---

## Key Changes

- Extend `TerrainDrawRegion`:
  - `LODSampleStep`
  - `MeshCellCountX`
  - `MeshCellCountZ`
  - `MeshSampleCountX`
  - `MeshSampleCountZ`
- Update packed tile mesh cache generation:
  - Compute max quadtree level from the provided node list.
  - Compute per-node `LODSampleStep`.
  - Build sampled X/Z cell coordinate arrays that always include first and end cell boundaries.
  - Generate vertices only at sampled coordinates.
  - Generate surface indices from mesh cell counts, not source heightfield cell counts.
  - Keep skirts appended after main indices.
- Extend stats/UI:
  - Track min/max LOD sample step across cached tile meshes.
  - Track rendered mesh cell count separately from covered terrain cell count.
  - Show LOD sample step range and rendered mesh cells in ImGui.
- Add validation:
  - `tools/verify_landscape_lod_tile_sampling.py`.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_lod_tile_sampling.py
```

Expected before implementation:
- Fails because LOD sample step fields, sampled coordinate generation, mesh cell stats, UI, and status records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL captures into:
  - `build\Win64-vs18\smoke-lod-sampling-d3d12`
  - `build\Win64-vs18\smoke-lod-sampling-vk`
  - `build\Win64-vs18\smoke-lod-sampling-d3d11`
  - `build\Win64-vs18\smoke-lod-sampling-gl`

Visual acceptance:
- Terrain, grid, sky, transparent quad, and quadtree overlay remain visible.
- Coarser distant quadtree leaves use fewer surface triangles than their covered source heightfield cells.
- Skirts stay enabled and the draw path does not create PSOs during frame rendering.

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
- Smoke:
  - D3D12: `build\Win64-vs18\smoke-lod-sampling-d3d12\landscape_lod_sampling_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-sampling-vk\landscape_lod_sampling_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-sampling-d3d11\landscape_lod_sampling_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-sampling-gl\landscape_lod_sampling_gl.png`
- Pixel check:
  - D3D12: `640x480`, `644` unique colors, `307200` non-dark pixels, `188129` terrain-like pixels, `2620` overlay-like pixels.
  - Vulkan: `640x480`, `644` unique colors, `307200` non-dark pixels, `188129` terrain-like pixels, `2620` overlay-like pixels.
  - D3D11: `640x480`, `644` unique colors, `307200` non-dark pixels, `188129` terrain-like pixels, `2620` overlay-like pixels.
  - OpenGL: `640x480`, `1885` unique colors, `307200` non-dark pixels, `162889` terrain-like pixels, `1739` overlay-like pixels.
- Visual check: D3D12 capture shows terrain, grid, transparent quad, sky, and quadtree overlay remain visible after level-dependent tile sampling.
- Implementation note: each packed tile mesh records source cell coverage and mesh cell count separately. Default max depth `4` produces LOD sample steps `16, 8, 4, 2, 1`, while skirts remain appended after main surface indices and use `BaseVertex`-relative index values.
