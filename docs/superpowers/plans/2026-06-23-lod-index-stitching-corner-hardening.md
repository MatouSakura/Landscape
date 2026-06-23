# LOD Index Stitching Corner Hardening

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Harden the first dynamic LOD index stitching path where two stitched fine edges meet at a terrain tile corner, and add a debug switch to compare stitched-index rendering against skirt-only rendering.

This stage keeps the v1 dynamic index-buffer approach. It does not add shader morphing, tessellation, or external heightmap streaming.

---

## Key Changes

- Update stitched index generation:
  - `AppendStitchedEdgeBand()` accepts clipped start/end samples.
  - If an adjacent edge is also stitched, the edge band skips that corner cell.
  - `AppendStitchedCornerPatch()` fills each stitched corner once.
  - Corner combinations:
    - West + South
    - West + North
    - East + South
    - East + North
- Extend `TerrainLODIndexStitchingStats`:
  - `StitchedCornerCount`
  - `CornerPatchIndexCount`
- Add a runtime comparison toggle:
  - `ForwardRenderer::SetTerrainLODIndexStitchingEnabled()`
  - ImGui checkbox: `Enable LOD index stitching`
  - When disabled, seam diagnostics and skirts remain available, but terrain draw regions do not use the dynamic stitched index buffer.
- Update validation:
  - Add `tools/verify_landscape_lod_index_stitching_corners.py`.
  - Keep old LOD index stitching verifier compatible.
  - Keep PSO stability checks.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_lod_index_stitching_corners.py
```

Expected before implementation:
- Fails because corner clipping, corner patch generation, corner stats, runtime toggle, UI checkbox, and documentation records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL default captures.
- D3D12 `mixed_lod` capture.

Visual acceptance:
- Terrain, sky, grid, transparent quad, and overlays remain visible.
- `mixed_lod` terrain remains visible with seam diagnostics.
- The debug checkbox allows disabling stitched indices while keeping skirts and overlays for comparison.

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
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_lod_index_stitching_corners.py
```

Smoke:

- D3D12: `build\Win64-vs18\smoke-lod-corner-hardening-d3d12\landscape_lod_corner_hardening_d3d12.png`
- Vulkan: `build\Win64-vs18\smoke-lod-corner-hardening-vk\landscape_lod_corner_hardening_vk.png`
- D3D11: `build\Win64-vs18\smoke-lod-corner-hardening-d3d11\landscape_lod_corner_hardening_d3d11.png`
- OpenGL: `build\Win64-vs18\smoke-lod-corner-hardening-gl\landscape_lod_corner_hardening_gl.png`
- D3D12 `mixed_lod`: `build\Win64-vs18\smoke-lod-corner-hardening-mixed-d3d12\landscape_lod_corner_hardening_mixed_d3d12.png`

Pixel check:

- D3D12: `640x480`, `non_dark=307200`, `ground_like=236192`, `grid_like=222801`, `cyan_like=3894`, `orange_like=1603`, `std_sum=38.56`.
- Vulkan: `640x480`, `non_dark=307200`, `ground_like=236192`, `grid_like=222801`, `cyan_like=3894`, `orange_like=1603`, `std_sum=38.56`.
- D3D11: `640x480`, `non_dark=307200`, `ground_like=236192`, `grid_like=222801`, `cyan_like=3894`, `orange_like=1603`, `std_sum=38.56`.
- OpenGL: `640x480`, `non_dark=307200`, `ground_like=194231`, `grid_like=289532`, `cyan_like=2646`, `orange_like=772`, `std_sum=48.52`.
- D3D12 `mixed_lod`: `640x480`, `non_dark=307200`, `ground_like=64310`, `grid_like=240730`, `cyan_like=1268`, `orange_like=455`, `std_sum=32.12`.

Result:

- Corner clipping and corner patch generation are implemented.
- `Enable LOD index stitching` toggles dynamic stitched-index terrain rendering while preserving skirts and seam diagnostics.
- PSO creation remains outside render-loop paths.
