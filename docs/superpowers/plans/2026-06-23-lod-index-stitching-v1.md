# LOD Index Stitching v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Convert the existing `TerrainLODStitching` seam records into the first real terrain index-stitching path. Mixed-LOD fine tiles should render through per-frame stitched indices along fine/coarse neighbor seams instead of relying only on visual diagnostics and skirts.

This stage keeps the current packed terrain vertex buffer and PSO model. It does not add tessellation, GPU morphing, or a streaming heightmap. It adds a dynamic stitched index buffer for visible selected leaves and uses it for both forward and shadow terrain draws.

---

## Key Changes

- Add `TerrainLODIndexStitching`:
  - Consumes `TerrainLODStitching` seam records.
  - Builds per-node edge masks for fine nodes that border coarser selected leaves.
  - Emits per-node stitched draw regions and stitch statistics.
  - Tracks stitched node count, stitched edge count, generated index count, and max stitch ratio.
- Extend terrain draw regions:
  - Mark regions that should draw from the per-frame stitched index buffer.
  - Preserve existing packed immutable vertex buffer.
  - Keep non-stitched nodes on the existing immutable packed index buffer.
- Update `TerrainPatchRenderer`:
  - Create a dynamic index buffer sized for visible stitched terrain draws.
  - Generate replacement surface indices for stitched fine-edge bands.
  - Append existing skirt indices after stitched surface indices when skirts are enabled.
  - Use stitched indices for both forward and shadow draws when a region is marked stitched.
- Update `ForwardRenderer`:
  - Build `TerrainLODIndexStitching` after visible selection and seam detection.
  - Submit stitched draw regions for affected nodes.
  - Prepare the dynamic stitched index buffer before shadow and forward passes.
  - Expose stitching stats in `ForwardRendererStats`.
- Update ImGui diagnostics:
  - Show stitched node count, stitched edge count, generated stitched index count, and max stitch ratio.
- Update validation:
  - Add `tools/verify_landscape_lod_index_stitching.py`.
  - Preserve existing PSO stability checks.
  - Keep D3D12/Vulkan/D3D11/OpenGL smoke and D3D12 `mixed_lod` smoke.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_lod_index_stitching.py
```

Expected before implementation:
- Fails because `TerrainLODIndexStitching`, dynamic stitched index buffer plumbing, renderer stats, UI stats, and status records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL default captures.
- D3D12 `mixed_lod` capture for mixed-LOD stitched edge visibility.

Visual acceptance:
- Terrain, sky, grid, transparent quad, and overlays remain visible.
- `mixed_lod` still shows seam diagnostics, but terrain fine edges are drawn through stitched index regions.
- Stitched node and index stats are nonzero in mixed-level views.
- PSO count does not grow during render.

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
- `tools\verify_landscape_lod_index_stitching.py`

Smoke:
- D3D12: `build\Win64-vs18\smoke-lod-index-stitching-d3d12\landscape_lod_index_stitching_d3d12.png`
- Vulkan: `build\Win64-vs18\smoke-lod-index-stitching-vk\landscape_lod_index_stitching_vk.png`
- D3D11: `build\Win64-vs18\smoke-lod-index-stitching-d3d11\landscape_lod_index_stitching_d3d11.png`
- OpenGL: `build\Win64-vs18\smoke-lod-index-stitching-gl\landscape_lod_index_stitching_gl.png`
- D3D12 `mixed_lod`: `build\Win64-vs18\smoke-lod-index-stitching-mixed-d3d12\landscape_lod_index_stitching_mixed_d3d12.png`

Pixel and visual checks:
- D3D12: `640x480`, `unique=636`, `non_dark=307200`, `terrain_like=40770`, `cyan_like=23991`, `orange_like=640`.
- Vulkan: `640x480`, `unique=636`, `non_dark=307200`, `terrain_like=40770`, `cyan_like=23991`, `orange_like=640`.
- D3D11: `640x480`, `unique=636`, `non_dark=307200`, `terrain_like=40770`, `cyan_like=23991`, `orange_like=640`.
- OpenGL: `640x480`, `unique=1877`, `non_dark=307200`, `terrain_like=25055`, `cyan_like=23114`, `orange_like=4805`.
- D3D12 `mixed_lod`: `640x480`, `unique=333`, `non_dark=307200`, `terrain_like=8132`, `cyan_like=5398`, `orange_like=349`.
- Visual check: D3D12 `mixed_lod` remains side-biased and keeps terrain plus selected leaf and seam overlay lines visible while stitched index regions are active.

Notes:
- This stage adds the first real dynamic index-stitching draw path for mixed LOD terrain leaves.
- Fine tile edge bands adjacent to coarser leaves are replaced by dynamic fan indices.
- Existing packed immutable terrain vertices and PSOs are preserved.
- Corner overlap hardening and shader morph comparison remain next-step work.
