# CPU Quadtree LOD Selection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first CPU-side quadtree node model and camera-driven LOD selection path for the procedural heightfield terrain.

**Architecture:** Keep the existing heightfield terrain patch renderer as the opaque render path for this slice. Add a separate quadtree data/selection layer that computes selected terrain leaves from camera position and node bounds, then visualize the selected leaves through a debug overlay and ImGui stats. This establishes the UE-style terrain component/section foundation before tiled rendering and crack fixing.

**Tech Stack:** C++20, Diligent Engine, HLSL/GLSL where needed, CMake, existing Python verification scripts.

---

## Scope

This milestone builds the quadtree control plane, not the final tiled terrain renderer.

In scope:
- CPU quadtree node data model.
- Deterministic full-tree build over the current heightfield patch extent.
- Camera-distance LOD selection.
- Optional frustum culling if it fits cleanly in the same data layer.
- Debug overlay showing selected quadtree leaves and LOD levels.
- ImGui stats and validation script.

Out of scope:
- Rendering each quadtree leaf as an independent terrain mesh.
- LOD crack fixing, skirts, morphing, geomorphing.
- Terrain streaming and external heightmap tiles.
- GPU culling or indirect drawing.

## Key Changes

### 1. Quadtree Data Model

Create `LandscapeEditor/src/TerrainQuadtree.hpp` and `TerrainQuadtree.cpp`.

Core types:

```cpp
struct TerrainQuadtreeDesc final
{
    Uint32 MaxDepth = 4;
    float  Extent   = 20.0f;
};

struct TerrainQuadtreeNode final
{
    Uint32 NodeIndex = 0;
    Uint32 ParentIndex = InvalidNodeIndex;
    Uint32 FirstChildIndex = InvalidNodeIndex;
    Uint32 Level = 0;
    Uint32 ChildMask = 0;
    float2 MinXZ = {};
    float2 MaxXZ = {};
    float2 CenterXZ = {};
    float  Radius = 0.0f;
};

struct TerrainQuadtreeSelection final
{
    std::vector<Uint32> SelectedNodeIndices;
    Uint32 MaxSelectedLevel = 0;
};
```

Behavior:
- Build a complete quadtree from root extent `[-Extent, +Extent]`.
- Children split in stable order: southwest, southeast, northwest, northeast.
- `InvalidNodeIndex` is `0xFFFFFFFFu`.
- Keep all nodes in one contiguous vector so future render items can reference nodes by stable index.

### 2. LOD Selection

Add selection API:

```cpp
void Build(const TerrainQuadtreeDesc& Desc);
void Select(const float3& CameraPosition, TerrainQuadtreeSelection& OutSelection) const;
const std::vector<TerrainQuadtreeNode>& GetNodes() const;
```

Initial split rule:
- Root always evaluated.
- A node splits when `Level < MaxDepth` and camera distance to node center is less than:

```cpp
SplitDistance = Extent * 2.2f / static_cast<float>(1u << Level);
```

- Otherwise the node becomes a selected leaf.
- Clamp camera Y out of the distance formula for first pass; use XZ distance only.

Acceptance:
- Near camera: more, smaller selected leaves.
- Far camera: fewer, larger selected leaves.
- Selection remains deterministic frame to frame for the same camera.

### 3. Debug Overlay

Create `LandscapeEditor/src/TerrainQuadtreeDebugRenderer.hpp` and `.cpp`.

Behavior:
- Render selected leaf bounds as world-space line rectangles at a slight Y offset above terrain, e.g. `Y = 0.08f`.
- Color by LOD level with a fixed palette.
- Use a dedicated PSO cache key: `ForwardDebug.TerrainQuadtree.LineList.v1`.
- No PSO creation inside `Render()`.
- OpenGL should either use the same HLSL path if smoke passes, or receive a GLSL-specific path like the terrain/sky/postprocess fixes.

### 4. ForwardRenderer Integration

Modify `ForwardRenderer`:
- Own `TerrainQuadtree` and `TerrainQuadtreeDebugRenderer`.
- Build quadtree during `Initialize()` using the terrain extent.
- Run `Select(View.CameraPosition, Selection)` each frame.
- Render debug overlay after terrain and before/with existing debug grid.
- Add stats:
  - `TerrainQuadtreeNodeCount`
  - `TerrainQuadtreeSelectedLeafCount`
  - `TerrainQuadtreeMaxDepth`
  - `TerrainQuadtreeMaxSelectedLevel`

Modify `LandscapeEditor` ImGui:
- Show the quadtree stats.
- Add one checkbox for `Show quadtree overlay`; default enabled for this milestone.

### 5. Validation and Docs

Add `tools/verify_landscape_quadtree_lod.py`.

Required checks:
- New quadtree files are in `LandscapeEditor/CMakeLists.txt`.
- `TerrainQuadtreeDesc`, `TerrainQuadtreeNode`, and `TerrainQuadtreeSelection` exist.
- Selection uses camera XZ distance and stable node indices.
- `ForwardRenderer` owns and updates quadtree selection.
- Debug overlay renderer has a PSO cache key and no PSO creation inside `Render()`.
- ImGui exposes quadtree stats and overlay toggle.
- `PROJECT_STATUS.md` records the milestone and next terrain-rendering step.

Update:
- `PROJECT_STATUS.md`
- This plan document after implementation with build/smoke results.

## Test Plan

Red-first:

```powershell
cd E:\Landscape
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_quadtree_lod.py
```

Expected before implementation:
- Fails because quadtree files, stats, and renderer integration do not exist.

Build:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build E:\Landscape\build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Static validation:

```powershell
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_stage4.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_stage5.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_stage6.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_forward_completion.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_heightfield.py
& "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" tools\verify_landscape_quadtree_lod.py
```

Smoke:
- D3D12 after first integration.
- Vulkan after debug overlay shader is added.
- D3D12, Vulkan, D3D11, and OpenGL before commit.

Visual acceptance:
- Heightfield terrain remains visible.
- Existing grid remains visible.
- Quadtree leaf rectangles are visible above terrain.
- Moving the camera changes selected leaf count and max selected level.
- PSO creation count does not grow per frame.

## Commit Plan

- Commit 1: Add quadtree data model and red/green validation.
- Commit 2: Add quadtree debug overlay renderer and UI stats.
- Commit 3: Update docs, run four-backend smoke, and push `origin/master`.

## Next Milestone After This

After quadtree selection is visible and stable, the next serious terrain step is tiled terrain rendering:
- Convert selected quadtree leaves into render items.
- Generate per-node mesh buffers or introduce a reusable grid mesh with per-node constants.
- Then address LOD cracks with skirts or morphing.
