# Selected Leaf Terrain Render Items Implementation Plan

## Summary

Convert CPU quadtree selected leaves from debug-only data into real opaque terrain render items. The first slice keeps the existing heightfield vertex/index buffers, but each selected leaf becomes its own `RenderItem` with a terrain draw region. Forward opaque and shadow passes then draw only the cells covered by each selected leaf.

This stage does not create independent per-tile mesh buffers and does not fix LOD cracks. It establishes the render queue and draw-region contract needed by the later tiled terrain renderer.

## Key Changes

- Extend `RenderQueue`:
  - Add `RenderItemKind::TerrainLeaf`.
  - Add `TerrainDrawRegion` with quadtree node index, LOD level, XZ bounds, first cell, and cell count.
  - Add `AddTerrainLeaf(region)`.
  - Stop submitting the old single full-patch terrain item from `ForwardRenderer`.
- Extend `TerrainPatchRenderer`:
  - Add `BuildDrawRegion(const TerrainQuadtreeNode&)`.
  - Add `Render(..., const TerrainDrawRegion&, ...)`.
  - Add `RenderShadow(..., const TerrainDrawRegion&)`.
  - Draw a rectangular leaf region by issuing row-based indexed draws with `FirstIndexLocation`.
  - Track last forward and shadow terrain draw-call counts.
- Extend `ShadowRenderer`:
  - Render shadow depth for the same terrain leaf render items used by forward opaque.
- Extend stats/UI:
  - `TerrainRenderItemCount`
  - `TerrainRenderedCellCount`
  - `TerrainForwardDrawCallCount`
  - `TerrainShadowDrawCallCount`
- Add validation:
  - `tools/verify_landscape_selected_leaf_render_items.py`
  - Static checks confirm selected leaves are submitted as terrain render items and render paths use draw regions.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_selected_leaf_render_items.py
```

Expected before implementation:
- Fails because `TerrainLeaf`, `TerrainDrawRegion`, selected-leaf queue submission, region rendering, and new stats do not exist.

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

Smoke:
- D3D12 after first integration.
- Vulkan after region rendering is stable.
- D3D12, Vulkan, D3D11, and OpenGL before commit.

Visual acceptance:
- Terrain, grid, sky, transparent quad, and quadtree overlay remain visible.
- Opaque terrain is drawn from selected leaf render items.
- Terrain render item count matches selected leaf count.
- PSO creation count does not grow during frame rendering.

## Validation Record

Completed on 2026-06-23.

Build:
- `LandscapeEditor` Release target built successfully with VS18 CMake:
  `cmake --build E:\Landscape\build\Win64-vs18 --config Release --target LandscapeEditor --parallel`

Static validation:
- `tools\verify_landscape_stage4.py` passed.
- `tools\verify_landscape_stage5.py` passed.
- `tools\verify_landscape_stage6.py` passed.
- `tools\verify_landscape_forward_completion.py` passed.
- `tools\verify_landscape_heightfield.py` passed.
- `tools\verify_landscape_quadtree_lod.py` passed.
- `tools\verify_landscape_selected_leaf_render_items.py` passed after this validation record was added.

Smoke:
- D3D12 capture: `build\Win64-vs18\smoke-selected-leaves-d3d12\landscape_selected_leaves_d3d12.png`
- Vulkan capture: `build\Win64-vs18\smoke-selected-leaves-vk\landscape_selected_leaves_vk.png`
- D3D11 capture: `build\Win64-vs18\smoke-selected-leaves-d3d11\landscape_selected_leaves_d3d11.png`
- OpenGL capture: `build\Win64-vs18\smoke-selected-leaves-gl\landscape_selected_leaves_gl.png`

Pixel and visual acceptance:
- All four captures are `640x480`.
- All four captures contain visible terrain, grid, sky, transparent quad, and quadtree overlay content.
- D3D12 visual inspection confirms the terrain remains continuous after switching forward opaque submission from one full-patch item to selected leaf render items.
- Pixel check counted terrain-like pixels and quadtree overlay-like line pixels in all four backend captures.

Implementation notes:
- `ForwardRenderer` now converts `TerrainQuadtreeSelection::SelectedNodeIndices` into opaque `TerrainLeaf` render items.
- `TerrainPatchRenderer::BuildDrawRegion()` maps each selected node's XZ bounds to heightfield cell ranges.
- Forward opaque and shadow passes both consume the same terrain render item list.
- Rendering still uses the existing immutable heightfield vertex/index buffers. Each selected leaf is drawn through row-based indexed draws using `FirstIndexLocation`; later stages can replace this with per-node buffers, a reusable tile grid, or indirect drawing.
- PSO creation remains initialization-time only.

## Assumptions

- The existing 64x64 heightfield and quadtree extent `20.0f` remain aligned.
- Leaf draw regions map quadtree XZ bounds to heightfield cell ranges.
- Row-based indexed draws are acceptable for this stage; a later stage can replace this with per-node mesh buffers, reusable tiled grids, or indirect drawing.
