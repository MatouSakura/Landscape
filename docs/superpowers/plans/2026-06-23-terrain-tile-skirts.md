# Terrain Tile Skirts Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-pass skirt geometry to each packed terrain tile mesh so future LOD transitions have a simple crack-hiding mechanism.

**Architecture:** `TerrainPatchRenderer` keeps the packed per-node tile mesh cache. During tile cache generation it appends four vertical skirt strips around every tile, records main surface index count and skirt index count separately, and draws either main-only or main+skirt indices based on a runtime debug toggle. This stage does not add geomorphing, LOD sampling reduction, or neighbor-aware stitching.

**Tech Stack:** C++20, Diligent Engine immutable vertex/index buffers, existing terrain shaders and PSOs, Dear ImGui diagnostics, Python static verification, VS18 CMake.

---

## Key Changes

- Extend `TerrainDrawRegion`:
  - `MainNumIndices`
  - `SkirtIndexCount`
- Extend `TerrainPatchRenderer`:
  - Add fixed `SkirtDepth = 1.25f`.
  - Generate west/east/south/north skirt geometry for every packed tile mesh.
  - Append skirt indices after main surface indices so main-only drawing stays possible.
  - Add `SetEnableSkirts(bool)` / `GetEnableSkirts()`.
  - Track packed skirt vertex and index counts.
  - Track rendered index count for accurate terrain triangle stats.
- Extend `ForwardRenderer` and ImGui:
  - Add `SetTerrainSkirtsEnabled(bool)`.
  - Add stats for enabled state, depth, skirt vertices, and skirt indices.
  - Add `Enable terrain skirts` checkbox.
- Add validation:
  - `tools/verify_landscape_tile_skirts.py`.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_tile_skirts.py
```

Expected before implementation:
- Fails because skirt fields, generation helper, draw toggle, UI, and status records do not exist.

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

Smoke:
- D3D12, Vulkan, D3D11, OpenGL captures into:
  - `build\Win64-vs18\smoke-tile-skirts-d3d12`
  - `build\Win64-vs18\smoke-tile-skirts-vk`
  - `build\Win64-vs18\smoke-tile-skirts-d3d11`
  - `build\Win64-vs18\smoke-tile-skirts-gl`

Visual acceptance:
- Terrain, grid, sky, transparent quad, and quadtree overlay remain visible.
- Skirt-enabled draw path does not introduce visible holes or backend failures.
- PSO creation remains initialization-time only.

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
- Smoke:
  - D3D12: `build\Win64-vs18\smoke-tile-skirts-d3d12\landscape_tile_skirts_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-tile-skirts-vk\landscape_tile_skirts_vk.png`
  - D3D11: `build\Win64-vs18\smoke-tile-skirts-d3d11\landscape_tile_skirts_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-tile-skirts-gl\landscape_tile_skirts_gl.png`
- Pixel check:
  - D3D12: `640x480`, `941` unique colors, `307200` non-dark pixels, `187037` terrain-like pixels, `2620` overlay-like pixels.
  - Vulkan: `640x480`, `941` unique colors, `307200` non-dark pixels, `187037` terrain-like pixels, `2620` overlay-like pixels.
  - D3D11: `640x480`, `941` unique colors, `307200` non-dark pixels, `187037` terrain-like pixels, `2620` overlay-like pixels.
  - OpenGL: `640x480`, `990` unique colors, `307200` non-dark pixels, `186849` terrain-like pixels, `1739` overlay-like pixels.
- Visual check: D3D12 capture shows terrain, grid, transparent quad, sky, and quadtree overlay remain visible with terrain tile skirts enabled.
- Implementation note: skirt geometry is appended after each tile's main surface indices, so `Enable terrain skirts` switches between main-only and main-plus-skirt indexed drawing without changing PSOs during frame rendering.
