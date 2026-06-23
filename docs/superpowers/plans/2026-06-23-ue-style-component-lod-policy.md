# UE-style Component LOD Policy Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development before production code. Keep build, validation, smoke, commit, and push records current.

**Goal:** Move the current hard-coded quadtree split rule toward an Unreal Engine Landscape-style component LOD policy: near camera positions select smaller terrain components with finer sampling, while far positions can remain larger components with coarser sampling.

This stage does not add UE asset import, Landscape Component serialization, streaming proxies, or crack-free stitching. It makes the current selected leaf system adjustable and observable so the next stitching/morphing work can target stable component sizes.

---

## Key Changes

- Add `TerrainComponentLODPolicy`:
  - `SplitDistanceScale = 2.2f`
  - `MaxSelectedLevel = 4`
- Update `TerrainQuadtree`:
  - Replace the hard-coded split formula with `ComputeSplitDistance(Level)`.
  - Stop splitting when `Level >= MaxSelectedLevel`.
  - Add `GetComponentWorldSize(Level)` so selected leaves can be described as component sizes.
  - Track `MinSelectedLevel` and `MaxSelectedLevel` in `TerrainQuadtreeSelection`.
- Update `ForwardRenderer` stats and controls:
  - `TerrainLODDistanceScale`
  - `TerrainMaxSelectableLODLevel`
  - `TerrainQuadtreeMinSelectedLevel`
  - `TerrainRootComponentWorldSize`
  - `TerrainFineComponentWorldSize`
  - `TerrainSelectedMinComponentWorldSize`
  - `TerrainSelectedMaxComponentWorldSize`
- Update ImGui:
  - Slider for `LOD distance scale`.
  - Slider for `Max terrain LOD`.
  - Display selected component world-size range.
- Update validation:
  - Add `tools/verify_landscape_component_lod_policy.py`.
  - Update the older quadtree verifier to accept the policy-based split rule.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_component_lod_policy.py
```

Expected before implementation:
- Fails because LOD policy, policy UI, selected component-size stats, and documentation records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL default captures.
- D3D12 `mixed_lod` scripted camera capture to inspect selected component sizes and transition overlays.

Visual acceptance:
- Near terrain remains fine and detailed.
- Far terrain remains larger selected quadtree leaves.
- UI can adjust the LOD distance scale and max terrain LOD without recreating PSOs in the render loop.
- Terrain, sky, grid, transparent quad, and debug overlays remain visible.

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
- Smoke:
  - D3D12: `build\Win64-vs18\smoke-component-lod-policy-d3d12\landscape_component_lod_policy_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-component-lod-policy-vk\landscape_component_lod_policy_vk.png`
  - D3D11: `build\Win64-vs18\smoke-component-lod-policy-d3d11\landscape_component_lod_policy_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-component-lod-policy-gl\landscape_component_lod_policy_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-component-lod-policy-mixed-d3d12\landscape_component_lod_policy_mixed_d3d12.png`
- Pixel check:
  - D3D12: `640x480`, `646` unique colors, `186081` terrain-like pixels, `2618` cyan diagnostic pixels.
  - Vulkan: `640x480`, `646` unique colors, `186081` terrain-like pixels, `2618` cyan diagnostic pixels.
  - D3D11: `640x480`, `646` unique colors, `186081` terrain-like pixels, `2618` cyan diagnostic pixels.
  - OpenGL: `640x480`, `1876` unique colors, `160977` terrain-like pixels, `7838` cyan diagnostic pixels.
  - D3D12 `mixed_lod`: `640x480`, `348` unique colors, `43870` terrain-like pixels, `697` cyan diagnostic pixels.
- Visual check: D3D12 `mixed_lod` keeps side-biased terrain visible with selected component overlay lines.
- Implementation note: the old hard-coded split rule is now `TerrainComponentLODPolicy`. `LOD distance scale` changes split distances, while `Max terrain LOD` caps the finest selectable quadtree level without rebuilding PSOs or terrain buffers.
