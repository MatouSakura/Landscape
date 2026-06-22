# Heightfield Terrain Patch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the flat terrain patch with a deterministic procedural heightfield patch that establishes the terrain data model for future heightmap loading and quadtree LOD.

**Architecture:** `TerrainHeightField` owns CPU height, normal, UV, and height statistics. `TerrainPatchRenderer` consumes that data to build one immutable indexed mesh used by both forward opaque and shadow passes. `ForwardRenderer` exposes terrain data stats to the existing ImGui diagnostics.

**Tech Stack:** C++20, Diligent Engine, HLSL, CMake, existing Python verification scripts.

---

## Task 1: Heightfield Verification Script

**Files:**
- Create: `tools/verify_landscape_heightfield.py`

- [x] Write a red-first static verification script for the heightfield milestone.
- [x] Verify it fails before implementation because `TerrainHeightField` and heightfield stats do not exist.

## Task 2: TerrainHeightField Data Layer

**Files:**
- Create: `LandscapeEditor/src/TerrainHeightField.hpp`
- Create: `LandscapeEditor/src/TerrainHeightField.cpp`
- Modify: `LandscapeEditor/CMakeLists.txt`

- [x] Add `TerrainHeightFieldDesc` with default 64 cells, 20 world-unit extent, and 2.5 height scale.
- [x] Add `TerrainHeightSampleStats` with min, max, and average height.
- [x] Add deterministic procedural generation using sin/cos wave terms and a radial term.
- [x] Add CPU central-difference normal generation.
- [x] Add normalized UV generation.
- [x] Register the files in `LandscapeEditor/CMakeLists.txt`.

## Task 3: Heightfield Terrain Rendering

**Files:**
- Modify: `LandscapeEditor/src/TerrainPatchRenderer.hpp`
- Modify: `LandscapeEditor/src/TerrainPatchRenderer.cpp`

- [x] Replace flat patch vertex generation with heightfield sampling.
- [x] Extend terrain vertices and input layout to `Pos + Normal + UV`.
- [x] Use the displaced vertex buffer in both forward opaque and shadow passes.
- [x] Update terrain shader color to include height, slope, and UV variation.
- [x] Rename terrain PSO cache keys to heightfield-specific names.

## Task 4: Runtime Diagnostics

**Files:**
- Modify: `LandscapeEditor/src/ForwardRenderer.hpp`
- Modify: `LandscapeEditor/src/ForwardRenderer.cpp`
- Modify: `LandscapeEditor/src/LandscapeEditor.cpp`

- [x] Add terrain cell count, sample count, min height, max height, and average height stats.
- [x] Refresh those stats in renderer initialization and frame stats.
- [x] Display the heightfield stats in the ImGui diagnostics panel.

## Task 5: Verification and Repository Hygiene

**Files:**
- Modify: `PROJECT_STATUS.md`

- [x] Run `verify_landscape_heightfield.py`.
- [x] Run stage 4, stage 5, stage 6, and forward completion verification.
- [x] Build `LandscapeEditor` Release with VS18 CMake.
- [x] Run D3D12, Vulkan, D3D11, and OpenGL smoke captures.
- [x] Commit and push to `origin/master`.

## Acceptance Criteria

- Terrain no longer uses a flat `PatchHeight`.
- Heightfield min/max/average stats are visible in the UI.
- Forward opaque and shadow pass both use the same height-displaced mesh.
- Existing forward renderer PSO stability rule remains valid: no PSO creation or cache lookup in `Render()`.
- Four backend smoke captures still render sky, terrain, grid, transparent quad, and postprocess output.

## Validation Record

- Build: `LandscapeEditor` Release target succeeded with VS18 CMake.
- Static validation passed:
  - `tools/verify_landscape_stage4.py`
  - `tools/verify_landscape_stage5.py`
  - `tools/verify_landscape_stage6.py`
  - `tools/verify_landscape_forward_completion.py`
  - `tools/verify_landscape_heightfield.py`
- Smoke captures passed on D3D12, Vulkan, D3D11, and OpenGL:
  - `build/Win64-vs18/smoke-heightfield-d3d12/landscape_heightfield_d3d12.png`
  - `build/Win64-vs18/smoke-heightfield-vk/landscape_heightfield_vk.png`
  - `build/Win64-vs18/smoke-heightfield-d3d11/landscape_heightfield_d3d11.png`
  - `build/Win64-vs18/smoke-heightfield-gl/landscape_heightfield_gl.png`
- OpenGL note: GL uses explicit render-target binding before clears and draws sky before opaque terrain so the sky pass does not cover the heightfield. D3D12, Vulkan, and D3D11 keep the normal opaque-then-sky order.
