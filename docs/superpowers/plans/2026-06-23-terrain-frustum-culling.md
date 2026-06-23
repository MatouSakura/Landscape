# Terrain Frustum Culling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Cull selected quadtree terrain leaves against the current camera frustum before submitting terrain render items, shadow items, and quadtree debug overlay lines.

**Architecture:** Keep the existing distance-based quadtree selection unchanged. `ForwardRenderer` will treat it as the candidate leaf set, extract a `ViewFrustum` from `RenderView.ViewProj`, and build a terrain AABB for each selected leaf from:

- `TerrainQuadtreeNode.MinXZ/MaxXZ`
- `TerrainPatchRenderer` min/max height
- skirt depth when terrain skirts are enabled

Visible leaves are copied into a separate `TerrainQuadtreeSelection` and are the only leaves submitted to the render queue and debug overlay when frustum culling is enabled. The full candidate selection remains tracked for stats.

This stage does not add occlusion culling, GPU culling, hierarchical quadtree pruning, or indirect drawing.

---

## Key Changes

- Extend `ForwardRenderer`:
  - Add `m_EnableTerrainFrustumCulling`.
  - Add `m_VisibleTerrainQuadtreeSelection`.
  - Add CPU AABB-vs-frustum helper using `AdvancedMath.hpp`.
  - Copy selected leaves into visible selection when culling is disabled.
  - Use visible selection for terrain render items and debug overlay.
- Extend `ForwardRendererStats`:
  - `TerrainFrustumCullingEnabled`
  - `TerrainQuadtreeCandidateLeafCount`
  - `TerrainQuadtreeVisibleLeafCount`
  - `TerrainFrustumCulledLeafCount`
- Extend UI:
  - `Enable terrain frustum culling`
  - Display candidate, visible, and culled leaf counts.
- Add validation:
  - `tools/verify_landscape_frustum_culling.py`.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_frustum_culling.py
```

Expected before implementation:
- Fails because culling helper, visible selection, stats, UI, and status records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL captures into:
  - `build\Win64-vs18\smoke-frustum-culling-d3d12`
  - `build\Win64-vs18\smoke-frustum-culling-vk`
  - `build\Win64-vs18\smoke-frustum-culling-d3d11`
  - `build\Win64-vs18\smoke-frustum-culling-gl`

Visual acceptance:
- Terrain, grid, sky, transparent quad, and debug overlays remain visible.
- Visible/submitted terrain leaf count is less than or equal to candidate leaf count.
- Culling can be toggled off from ImGui.
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
- Smoke:
  - D3D12: `build\Win64-vs18\smoke-frustum-culling-d3d12\landscape_frustum_culling_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-frustum-culling-vk\landscape_frustum_culling_vk.png`
  - D3D11: `build\Win64-vs18\smoke-frustum-culling-d3d11\landscape_frustum_culling_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-frustum-culling-gl\landscape_frustum_culling_gl.png`
- Pixel check:
  - D3D12: `640x480`, `646` unique colors, `307200` non-dark pixels, `186081` terrain-like pixels, `5497` overlay-like pixels.
  - Vulkan: `640x480`, `646` unique colors, `307200` non-dark pixels, `186081` terrain-like pixels, `5497` overlay-like pixels.
  - D3D11: `640x480`, `646` unique colors, `307200` non-dark pixels, `186081` terrain-like pixels, `5497` overlay-like pixels.
  - OpenGL: `640x480`, `1876` unique colors, `307200` non-dark pixels, `160977` terrain-like pixels, `3418` overlay-like pixels.
- Visual check: D3D12 capture shows terrain, grid, transparent quad, sky, selected leaf bounds, and cyan skirt edge diagnostics remain visible with culling enabled.
- Implementation note: culling uses Diligent `AdvancedMath.hpp` frustum extraction and AABB visibility checks. The render queue and quadtree debug overlay consume `m_VisibleTerrainQuadtreeSelection`, while the raw distance-based selection remains available as candidate stats.
