# Packed Tile Mesh Cache Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace row-based selected-leaf region draws with real per-node terrain tile mesh ranges packed into shared immutable vertex and index buffers.

**Architecture:** `TerrainPatchRenderer` will build the procedural heightfield once, then generate one tile mesh range for each quadtree node. `ForwardRenderer` keeps converting selected leaves into `TerrainLeaf` render items, but each item now points at a packed tile mesh range with `BaseVertex`, `FirstIndexLocation`, and `NumIndices`. Forward opaque and shadow passes both draw the same tile mesh range.

**Tech Stack:** C++20, Diligent Engine immutable vertex/index buffers, existing HLSL/GLSL terrain shaders, existing Python static verification scripts, VS18 CMake.

---

## File Structure

- Modify `LandscapeEditor/src/RenderQueue.hpp`
  - Extend `TerrainDrawRegion` with packed mesh draw fields.
- Modify `LandscapeEditor/src/TerrainPatchRenderer.hpp`
  - Change `Initialize()` to receive quadtree nodes.
  - Expose packed mesh cache stats.
  - Replace row draw helper with packed tile mesh draw helper.
- Modify `LandscapeEditor/src/TerrainPatchRenderer.cpp`
  - Generate packed per-node tile vertices and indices.
  - Map selected node to prebuilt mesh range.
  - Draw each terrain leaf with one indexed draw.
- Modify `LandscapeEditor/src/ForwardRenderer.cpp`
  - Build quadtree before initializing terrain renderer.
  - Preserve selected leaf render item submission.
  - Update stats from tile mesh cache.
- Modify `LandscapeEditor/src/ForwardRenderer.hpp`
  - Add tile mesh cache stats.
- Modify `LandscapeEditor/src/LandscapeEditor.cpp`
  - Show tile mesh cache stats in ImGui.
- Create `tools/verify_landscape_packed_tile_mesh_cache.py`
  - Red-first static verification for packed tile mesh cache behavior.
- Update `PROJECT_STATUS.md`
  - Record completed packed tile mesh cache milestone and next work.

## Tasks

### Task 1: Red Verification Script

**Files:**
- Create: `tools/verify_landscape_packed_tile_mesh_cache.py`

- [ ] **Step 1: Write failing verification**

The script must check:
- `TerrainDrawRegion` has `BaseVertex`, `FirstIndexLocation`, and `NumIndices`.
- `TerrainPatchRenderer::Initialize()` accepts quadtree nodes.
- `TerrainPatchRenderer` has a packed tile mesh range/cache type.
- Tile cache build appends vertices and indices per quadtree node.
- Forward and shadow terrain draws use `BaseVertex`, `FirstIndexLocation`, and `NumIndices`.
- Row-based `DrawRegionIndexed` is removed.
- UI and status docs mention packed tile mesh cache.

- [ ] **Step 2: Run red verification**

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_packed_tile_mesh_cache.py
```

Expected: FAIL because packed tile mesh cache fields and docs do not exist yet.

### Task 2: Packed Tile Mesh Data Model

**Files:**
- Modify: `LandscapeEditor/src/RenderQueue.hpp`
- Modify: `LandscapeEditor/src/TerrainPatchRenderer.hpp`

- [ ] **Step 1: Extend terrain draw region**

Add these fields to `TerrainDrawRegion`:

```cpp
Uint32 BaseVertex         = 0;
Uint32 FirstIndexLocation = 0;
Uint32 NumIndices         = 0;
```

- [ ] **Step 2: Add renderer cache stats**

Add `TerrainTileMeshRange` and stats methods in `TerrainPatchRenderer`:

```cpp
struct TerrainTileMeshRange final
{
    TerrainDrawRegion Region;
    Uint32            VertexCount = 0;
};
```

Add getters:

```cpp
Uint32 GetTileMeshCount() const;
Uint32 GetPackedTileVertexCount() const;
Uint32 GetPackedTileIndexCount() const;
```

### Task 3: Build Packed Tile Mesh Cache

**Files:**
- Modify: `LandscapeEditor/src/TerrainPatchRenderer.cpp`

- [ ] **Step 1: Change initialization**

Change terrain renderer initialization to:

```cpp
void TerrainPatchRenderer::Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain, PSOCache& PSOCache, const std::vector<TerrainQuadtreeNode>& QuadtreeNodes)
```

- [ ] **Step 2: Build all tile ranges**

For each quadtree node:
- Map node bounds to heightfield cell range.
- Append only that node's tile vertices.
- Append local tile indices.
- Store `BaseVertex`, `FirstIndexLocation`, `NumIndices`, and `VertexCount`.

- [ ] **Step 3: Preserve existing shader path**

Keep the existing vertex layout and terrain/shadow PSO keys. This stage changes mesh packing only, not shader semantics.

### Task 4: Draw From Tile Mesh Ranges

**Files:**
- Modify: `LandscapeEditor/src/TerrainPatchRenderer.cpp`
- Modify: `LandscapeEditor/src/ForwardRenderer.cpp`
- Modify: `LandscapeEditor/src/ShadowRenderer.cpp`

- [ ] **Step 1: Replace row draw helper**

Remove `DrawRegionIndexed()` and add one indexed draw per leaf:

```cpp
DrawAttrs.NumIndices         = Region.NumIndices;
DrawAttrs.FirstIndexLocation = Region.FirstIndexLocation;
DrawAttrs.BaseVertex         = Region.BaseVertex;
```

- [ ] **Step 2: Initialize order**

Build the quadtree before terrain renderer initialization:

```cpp
TerrainQuadtreeDesc QuadtreeDesc;
m_TerrainQuadtree.Build(QuadtreeDesc);
m_TerrainPatchRenderer.Initialize(pDevice, pSwapChain, m_PSOCache, m_TerrainQuadtree.GetNodes());
```

### Task 5: Stats, Docs, Validation

**Files:**
- Modify: `LandscapeEditor/src/ForwardRenderer.hpp`
- Modify: `LandscapeEditor/src/ForwardRenderer.cpp`
- Modify: `LandscapeEditor/src/LandscapeEditor.cpp`
- Modify: `PROJECT_STATUS.md`
- Modify: `docs/superpowers/plans/2026-06-23-packed-tile-mesh-cache.md`

- [ ] **Step 1: Add stats**

Add:

```cpp
Uint32 TerrainTileMeshCount = 0;
Uint32 TerrainPackedVertexCount = 0;
Uint32 TerrainPackedIndexCount = 0;
```

- [ ] **Step 2: Show stats in UI**

Display tile mesh count, packed vertices, and packed indices in the existing diagnostics panel.

- [ ] **Step 3: Validate**

Run:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build E:\Landscape\build\Win64-vs18 --config Release --target LandscapeEditor --parallel
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_stage4.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_stage5.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_stage6.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_forward_completion.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_heightfield.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_quadtree_lod.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_selected_leaf_render_items.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_packed_tile_mesh_cache.py
```

- [ ] **Step 4: Smoke**

Run D3D12, Vulkan, D3D11, and OpenGL captures into:
- `build\Win64-vs18\smoke-packed-tiles-d3d12`
- `build\Win64-vs18\smoke-packed-tiles-vk`
- `build\Win64-vs18\smoke-packed-tiles-d3d11`
- `build\Win64-vs18\smoke-packed-tiles-gl`

- [ ] **Step 5: Commit and push**

```powershell
git add LandscapeEditor/src PROJECT_STATUS.md docs/superpowers/plans/2026-06-23-packed-tile-mesh-cache.md tools/verify_landscape_packed_tile_mesh_cache.py
git commit -m "Add packed terrain tile mesh cache"
git push origin master
```

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
- `tools\verify_landscape_selected_leaf_render_items.py` passed.
- `tools\verify_landscape_packed_tile_mesh_cache.py` passed after this validation record was added.

Smoke:
- D3D12 capture: `build\Win64-vs18\smoke-packed-tiles-d3d12\landscape_packed_tiles_d3d12.png`
- Vulkan capture: `build\Win64-vs18\smoke-packed-tiles-vk\landscape_packed_tiles_vk.png`
- D3D11 capture: `build\Win64-vs18\smoke-packed-tiles-d3d11\landscape_packed_tiles_d3d11.png`
- OpenGL capture: `build\Win64-vs18\smoke-packed-tiles-gl\landscape_packed_tiles_gl.png`

Pixel and visual acceptance:
- All four captures are `640x480`.
- All four captures contain visible terrain, grid, sky, transparent quad, and quadtree overlay content.
- D3D12 visual inspection confirms the heightfield terrain remains continuous after moving from row-based region draws to packed per-node tile mesh ranges.
- Pixel check counted terrain-like pixels and quadtree overlay-like line pixels in all four backend captures.

Implementation notes:
- `TerrainPatchRenderer` now builds a packed tile mesh range for every quadtree node during initialization.
- `TerrainDrawRegion` carries `BaseVertex`, `FirstIndexLocation`, and `NumIndices`.
- Selected leaf render items now draw one indexed tile mesh range each in both forward opaque and shadow passes.
- The existing HLSL/GLSL terrain shader paths and PSO keys are unchanged.
- PSO creation remains initialization-time only.
