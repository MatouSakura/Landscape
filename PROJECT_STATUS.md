# Landscape Project Status

Last updated: 2026-06-22

This document records what has been done, what will be done next, and the current unresolved issues for the Landscape terrain project.

## Project Goal

Build a cross-platform terrain rendering system inspired by Unreal Engine Landscape, using a mature rendering foundation instead of writing a graphics API abstraction from scratch.

The first target is a standalone terrain prototype based on Diligent Engine. The long-term goal is a reusable terrain module with quadtree LOD, heightmap tiles, material layers, streaming, debugging tools, and collision support.

## Current Repository

- Local path: `E:\Landscape`
- GitHub repository: `https://github.com/MatouSakura/Landscape`
- Base framework: Diligent Engine
- Current branch: `master`
- Current base commit: `768ad6c`
- `origin`: `https://github.com/MatouSakura/Landscape.git`
- `upstream`: `https://github.com/DiligentGraphics/DiligentEngine.git`

## Completed Work

### Repository Setup

- Cloned Diligent Engine into `E:\Landscape`.
- Pulled all recursive submodules.
- Created GitHub repository `MatouSakura/Landscape`.
- Pushed the initial Diligent Engine base to GitHub.
- Kept the original Diligent remote as `upstream`.

### Framework Decision

Diligent Engine was selected as the rendering base because it provides a mature cross-platform rendering abstraction while still allowing custom low-level rendering systems.

Primary reasons:

- C++ and CMake based.
- Supports Vulkan and D3D12, which are the most relevant backends for this project.
- Also supports other backends such as D3D11, OpenGL, Metal, and WebGPU.
- Allows terrain code to be written above a unified rendering interface.
- Avoids spending the early project on writing a custom RHI.

### Hardware / RTXNS Finding

RTXNS was cloned and built separately under `E:\RTXNX`, but it is not suitable as the AMD terrain project base.

Finding:

- The local machine uses AMD Radeon RX 7900 XT.
- RTXNS requires `VK_NV_cooperative_vector`.
- AMD drivers do not expose `VK_NV_cooperative_vector`.
- RTXNS can compile locally, but its cooperative-vector samples cannot run on this AMD machine.

This is treated as a reference-only project, not the Landscape runtime base.

## Planned Technical Stack

### Core

- Language: C++20
- Build system: CMake
- Rendering foundation: Diligent Engine
- Primary graphics backends: Vulkan and D3D12
- Shader language: HLSL first, with backend translation handled by the framework where possible

### Terrain Runtime

- Terrain partitioning: quadtree
- Terrain rendering unit: patch / tile
- LOD strategy: distance-based quadtree LOD first
- Crack handling: skirt or LOD morphing
- Culling: CPU frustum culling first, GPU culling later
- Rendering path: indexed patch mesh first, indirect draw later

### Terrain Data

- Height data: R16 or R32F heightmap
- Normal data: generated from heightmap, either offline or in shader
- Material data: splat map / layer weight map
- Large terrain: tiled heightmap package later

### Tools

- Frame debugging: RenderDoc
- Runtime debug UI: Dear ImGui
- Physics later: Jolt Physics height field

## Roadmap

### Phase 0: Validate Framework

- Build Diligent Engine locally.
- Run at least one Diligent sample with Vulkan.
- Run at least one Diligent sample with D3D12.
- Confirm AMD RX 7900 XT behaves correctly on both backends.

### Phase 1: Create Landscape Prototype App

- Add a new sample or app named `LandscapePrototype`.
- Keep it separate from upstream Diligent samples where possible.
- Open a window and render a basic grid.
- Add camera movement and a debug UI.

### Phase 2: Basic Heightmap Terrain

- Load a single heightmap.
- Generate terrain vertex/index buffers.
- Render a fixed-size terrain patch.
- Add basic normal calculation.
- Add a simple terrain material.

### Phase 3: Quadtree LOD

- Split terrain into quadtree nodes.
- Select LOD based on camera distance.
- Add frustum culling.
- Render selected leaf nodes.
- Add debug overlay for quadtree nodes and LOD levels.

### Phase 4: LOD Crack Fixing

- Investigate skirt-based crack hiding.
- Investigate LOD morphing.
- Pick one method for the first stable implementation.
- Verify transitions while moving the camera.

### Phase 5: Terrain Materials

- Add layer weight maps.
- Add multiple terrain textures.
- Implement basic splat blending.
- Add material debug views.

### Phase 6: Streaming

- Convert heightmap into tiles.
- Add tile cache.
- Load and unload tiles around the camera.
- Move IO work to background tasks.

### Phase 7: Advanced Rendering

- Add GPU culling.
- Add indirect drawing.
- Consider virtual texture or clipmap techniques.
- Add shadow and lighting integration.

### Phase 8: Collision and Queries

- Implement CPU ray-height query.
- Add terrain picking.
- Add Jolt height field integration if needed.

## Open Issues / Bugs

| ID | Status | Severity | Area | Description | Next Action |
| --- | --- | --- | --- | --- | --- |
| BUG-001 | Open | High | Runtime support | RTXNS cannot run core samples on AMD because AMD lacks `VK_NV_cooperative_vector`. | Do not use RTXNS as the runtime base. Keep it as reference only. |
| BUG-002 | Open | Medium | Validation | Diligent Engine has been cloned and pushed, but has not yet been built in `E:\Landscape`. | Configure and build Diligent with VS2022 CMake. |
| BUG-003 | Open | Medium | Validation | No Diligent sample has been run yet on AMD RX 7900 XT. | Run Vulkan and D3D12 samples after build. |
| BUG-004 | Open | Medium | Architecture | The final location for custom Landscape code is not decided yet: inside Diligent samples, separate app folder, or separate repository using Diligent as dependency. | Decide before adding prototype code. |
| BUG-005 | Open | Low | Git tooling | GitHub CLI is not installed. GitHub auth currently uses Git Credential Manager over HTTPS. | Optional: install GitHub CLI later if repository automation becomes frequent. |
| BUG-006 | Open | Low | Git tooling | SSH push is not configured because no GitHub SSH key exists on this machine. | HTTPS push works; configure SSH only if needed. |

## Architecture Decisions

### Decision 001: Use Diligent Engine

Use Diligent Engine as the cross-platform rendering foundation.

Reasoning:

- The project goal is terrain rendering, not writing a full cross-platform RHI.
- Diligent gives enough control for custom rendering systems.
- Vulkan and D3D12 can both be supported without duplicating all rendering code.

### Decision 002: Terrain System First, Editor Later

The first version should be runtime-only.

Do first:

- Camera.
- Heightmap loading.
- Patch rendering.
- Quadtree LOD.
- Debug visualization.

Do later:

- Terrain editing.
- Sculpting.
- Painting.
- Asset packaging tools.

### Decision 003: CPU Quadtree Before GPU-Driven Rendering

Start with CPU-side quadtree selection.

Reasoning:

- Easier to debug.
- Easier to compare with UE-style component and section concepts.
- GPU culling and indirect draw can be added once the terrain data model is stable.

## Useful Commands

### Clone With Submodules

```powershell
git clone --recursive https://github.com/MatouSakura/Landscape.git E:\Landscape
```

### Update Submodules

```powershell
cd E:\Landscape
git submodule update --init --recursive
```

### Sync From Diligent Upstream

```powershell
cd E:\Landscape
git fetch upstream
git merge upstream/master
git push
```

### Build With Visual Studio 2022 CMake

```powershell
cd E:\Landscape
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build\Win64 -G "Visual Studio 17 2022" -A x64
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64 --config Release --parallel
```

## Next Immediate Steps

1. Build Diligent Engine in `E:\Landscape`.
2. Run a Vulkan sample.
3. Run a D3D12 sample.
4. Decide where `LandscapePrototype` should live.
5. Add the first prototype app.
6. Render a flat grid.
7. Replace the flat grid with a heightmap terrain patch.
8. Add quadtree LOD selection.

## Notes

- Keep upstream Diligent code as clean as possible.
- Prefer adding Landscape-specific code in isolated folders.
- Avoid editing Diligent internals unless the terrain system truly requires it.
- Keep unresolved issues in this document until they are verified as fixed.
