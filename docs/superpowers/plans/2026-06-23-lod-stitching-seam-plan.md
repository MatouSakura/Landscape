# LOD Stitching Seam Plan v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Create the first neighbor-aware LOD stitching data layer for selected terrain leaves. The system should identify adjacent mixed-LOD edges as formal seam records rather than only drawing red debug lines. This prepares the renderer for a later index-stitching or shader-morph pass.

This stage does not yet rewrite terrain tile indices or add GPU vertex morphing. It establishes the seam plan that those later steps will consume.

---

## Key Changes

- Add `TerrainLODStitching`:
  - `TerrainLODSeamSide`
  - `TerrainLODSeamEdge`
  - `TerrainLODStitchingStats`
  - `Build(nodes, selection)`
  - `GetSeams()`
  - `GetStats()`
- Seam records identify:
  - fine node
  - coarse node
  - fine-side edge
  - world-space XZ start/end
  - LOD delta
  - stitch ratio
  - seam length
- Integrate into `ForwardRenderer`:
  - Build seam plan from visible selected leaves every frame.
  - Expose seam count, max delta, max stitch ratio, and total seam length in stats.
- Integrate into debug overlay:
  - Red mixed-LOD transition lines are drawn from `TerrainLODStitching` records.
  - Existing leaf bounds and skirt edge diagnostics remain unchanged.
- Update ImGui:
  - Display LOD stitching seam count, max delta, max stitch ratio, and seam length.
- Add validation:
  - `tools/verify_landscape_lod_stitching.py`.
  - Update older LOD transition diagnostics verifier to accept seam-plan driven transition edges.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_lod_stitching.py
```

Expected before implementation:
- Fails because `TerrainLODStitching`, CMake entries, renderer integration, debug renderer seam-plan rendering, UI stats, and documentation records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL default captures.
- D3D12 `mixed_lod` capture for visible seam-plan transition edges.

Visual acceptance:
- Terrain, sky, grid, transparent quad, and overlays remain visible.
- Mixed LOD transition edges remain visible, now backed by `TerrainLODStitching`.
- Seam stats are nonzero in mixed-level views.
- No PSO creation happens during render.

## Validation Record

Completed on 2026-06-23.

Build:
- `LandscapeEditor` Release target built successfully with VS18 CMake.

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

Smoke:
- D3D12: `build\Win64-vs18\smoke-lod-stitching-d3d12\landscape_lod_stitching_d3d12.png`
- Vulkan: `build\Win64-vs18\smoke-lod-stitching-vk\landscape_lod_stitching_vk.png`
- D3D11: `build\Win64-vs18\smoke-lod-stitching-d3d11\landscape_lod_stitching_d3d11.png`
- OpenGL: `build\Win64-vs18\smoke-lod-stitching-gl\landscape_lod_stitching_gl.png`
- D3D12 `mixed_lod`: `build\Win64-vs18\smoke-lod-stitching-mixed-d3d12\landscape_lod_stitching_mixed_d3d12.png`

Pixel and visual checks:
- All captures are `640x480` and keep terrain visible.
- D3D12, Vulkan, D3D11, and OpenGL default captures have full-frame non-dark coverage.
- The `mixed_lod` D3D12 capture has 5536 cyan diagnostic pixels and 349 orange seam-transition pixels after postprocess tone mapping.
- The `mixed_lod` visual check shows selected leaf and seam overlay lines on the side-biased terrain view.

Notes:
- The seam plan is metadata first: it records fine/coarse neighbor edges, LOD delta, stitch ratio, and seam length.
- Actual edge index rewriting and shader morphing remain deferred to the next crack-fixing step.
