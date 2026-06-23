# LOD Transition Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Add debug visualization for mixed-density terrain LOD transitions so skirt edges and potential crack-prone boundaries are visible before implementing stitching or morphing.

**Architecture:** Reuse `TerrainQuadtreeDebugRenderer` and its existing line-list PSO. The renderer will keep drawing selected leaf bounds, and optionally append two diagnostic overlays:

- Skirt edges: cyan tile perimeter lines for every selected leaf, showing where the tile skirts exist.
- LOD transition edges: red/orange line segments where two selected leaves share an edge but have different LOD levels.

The first version computes adjacency on CPU from selected quadtree leaf bounds. It is intentionally O(N^2) because the selected leaf count is capped by the quadtree depth and remains small. This stage does not change terrain geometry, stitching, morphing, or height sampling.

---

## Key Changes

- Extend `TerrainQuadtreeDebugRenderer`:
  - Add `TerrainQuadtreeDebugStats`.
  - Add `SetShowSkirtEdges(bool)` and `SetShowLODTransitionEdges(bool)`.
  - Add skirt edge line generation for selected leaves.
  - Add adjacency/overlap detection for shared edges with different `Level`.
  - Track leaf bounds, skirt edge, LOD transition edge, and total line counts.
- Extend `ForwardRendererStats`:
  - `TerrainDebugLeafBoundLineCount`
  - `TerrainDebugSkirtEdgeCount`
  - `TerrainDebugLODTransitionEdgeCount`
  - `TerrainDebugLineVertexCount`
- Extend UI:
  - `Show skirt edge overlay`
  - `Show LOD transition overlay`
  - Display debug edge counts.
- Add validation:
  - `tools/verify_landscape_lod_transition_diagnostics.py`.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_lod_transition_diagnostics.py
```

Expected before implementation:
- Fails because transition diagnostic stats, toggles, edge generation, UI, and status records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL captures into:
  - `build\Win64-vs18\smoke-lod-transitions-d3d12`
  - `build\Win64-vs18\smoke-lod-transitions-vk`
  - `build\Win64-vs18\smoke-lod-transitions-d3d11`
  - `build\Win64-vs18\smoke-lod-transitions-gl`

Visual acceptance:
- Terrain, grid, sky, transparent quad, and normal quadtree bounds remain visible.
- Skirt edge overlay is visible when enabled.
- LOD transition edge overlay is visible when selected leaves contain mixed levels.
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
- Smoke:
  - D3D12: `build\Win64-vs18\smoke-lod-transitions-d3d12\landscape_lod_transitions_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-transitions-vk\landscape_lod_transitions_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-transitions-d3d11\landscape_lod_transitions_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-transitions-gl\landscape_lod_transitions_gl.png`
- Pixel check:
  - D3D12: `640x480`, `646` unique colors, `307200` non-dark pixels, `186081` terrain-like pixels, `5497` overlay-like pixels.
  - Vulkan: `640x480`, `646` unique colors, `307200` non-dark pixels, `186081` terrain-like pixels, `5497` overlay-like pixels.
  - D3D11: `640x480`, `646` unique colors, `307200` non-dark pixels, `186081` terrain-like pixels, `5497` overlay-like pixels.
  - OpenGL: `640x480`, `1876` unique colors, `307200` non-dark pixels, `160977` terrain-like pixels, `3418` overlay-like pixels.
- Visual check: D3D12 capture shows terrain, grid, transparent quad, sky, selected leaf bounds, and cyan skirt edge diagnostics.
- Implementation note: the default smoke camera did not produce red mixed-LOD transition pixels, so the next validation improvement should add a scripted camera/smoke position that forces adjacent selected leaves at different levels.
