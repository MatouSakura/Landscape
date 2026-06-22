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
- Current base commit: `23e4db5`
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

### Forward Renderer Planning

- Added the first-slice forward debug pipeline design:
  `docs/superpowers/specs/2026-06-22-forward-debug-pipeline-design.md`
- Added the broader forward renderer architecture design:
  `docs/superpowers/specs/2026-06-22-forward-renderer-architecture-design.md`
- Added the implementation plan for the first runnable editor sample:
  `docs/superpowers/plans/2026-06-22-landscape-editor-forward-debug.md`
- Added the implementation plan for the first flat debug grid:
  `docs/superpowers/plans/2026-06-22-forward-debug-grid.md`
- Added the complete forward pipeline implementation plan:
  `docs/superpowers/plans/2026-06-22-forward-pipeline-completion.md`
- Added the camera, `RenderView`, and frame resources implementation plan:
  `docs/superpowers/plans/2026-06-22-camera-render-view-frame-resources.md`
- Added the `ForwardRenderer`, `RenderQueue`, and `PSOCache` implementation plan:
  `docs/superpowers/plans/2026-06-22-forward-renderer-queue-pso-cache.md`
- Added the sun light and CSM shadow implementation plan:
  `docs/superpowers/plans/2026-06-22-sun-csm-shadow.md`
- The planned path is now to expand `LandscapeEditor` into a complete first forward renderer with camera-driven frame resources, render queues, PSO cache, terrain patch rendering, sun light with four-cascade shadows, procedural sky, transparent/debug/postprocess passes, and runtime debug UI.

### Framework Build / Runtime Validation

Validation completed on 2026-06-22 with Visual Studio Community 2026 and its bundled CMake.

Working configure command:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build\Win64-vs18 -G "Visual Studio 18 2026" -A x64 -DPython3_EXECUTABLE="C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
```

Working build command:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target Tutorial01_HelloTriangle --parallel
```

Result:

- `Tutorial01_HelloTriangle` built successfully in Release.
- CMake detected `D3D11_SUPPORTED: TRUE`, `D3D12_SUPPORTED: TRUE`, `GL_SUPPORTED: TRUE`, and `VULKAN_SUPPORTED: TRUE`.
- D3D11, D3D12, Vulkan, and OpenGL smoke runs all rendered a triangle and captured a PNG.
- D3D12 runtime log used adapter `AMD Radeon RX 7900 XT`.
- Smoke captures:
  - `build\Win64-vs18\smoke-d3d11\hello_d3d11.png`
  - `build\Win64-vs18\smoke-d3d12\hello_d3d12.png`
  - `build\Win64-vs18\smoke-vk\hello_vk.png`
  - `build\Win64-vs18\smoke-gl\hello_gl.png`

Environment note:

- Visual Studio 2022 BuildTools CMake could configure only after passing the bundled Python executable, but the build failed because that VS2022 BuildTools install is missing ATL (`atlbase.h`).
- The VS2022 configure also reported D3D11/D3D12 unavailable for the same ATL reason.
- The VS 18 Community toolchain has ATL installed and is the current working build path.

### LandscapeEditor Bring-Up

`LandscapeEditor` has been added under the root-owned `LandscapeEditor/` directory.

Current implementation:

- Registers a `LandscapeEditor` Diligent sample target.
- Opens through the existing Diligent `SampleBase` application path.
- Adds `ForwardDebugPipeline` as the first rendering boundary.
- Owns a Diligent `FirstPersonCamera`.
- Builds a `RenderView` each frame from camera, swap chain, and backend data.
- Updates camera constants through `FrameResources`.
- Calls `ForwardRenderer::Render()` instead of owning debug draw calls directly.
- Submits the world-space grid through a `RenderQueue` debug item.
- Warms up the debug grid PSO through the project `PSOCache`.
- Renders a procedural HLSL world-space debug grid through `ForwardDebugPipeline`.
- Renders a CPU-generated flat terrain patch through `ForwardOpaque`.
- Runs a four-cascade shadow pass before forward opaque rendering.
- Updates sun light constants and samples the first CSM shadow map in the terrain shader.
- Keeps heightmaps, full cascade selection/stabilization, terrain materials, sky, transparent, and postprocess deferred until the first forward renderer is complete.

Validation completed on 2026-06-22:

- Configure: Visual Studio 18 CMake, `build\Win64-vs18`.
- Build: `LandscapeEditor` Release target succeeded.
- Stage-2 validation script: `tools\verify_landscape_stage2.py`.
- Stage-2 D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage2-d3d12\landscape_editor_stage2_d3d12.png`.
- Stage-2 pixel check: `640x480`, 3 unique colors, 984 bright axis pixels, 23686 grid pixels.
- Stage-3 validation script: `tools\verify_landscape_stage3.py`.
- Stage-3 D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage3-d3d12\landscape_editor_stage3_d3d12.png`.
- Stage-3 Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage3-vk\landscape_editor_stage3_vk.png`.
- Stage-3 pixel check: both captures are `640x480`, 3 unique colors, 984 bright axis pixels, 24670 non-background grid pixels.
- Stage-4 validation script: `tools\verify_landscape_stage4.py`.
- Stage-4 D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage4-d3d12\landscape_editor_stage4_d3d12.png`.
- Stage-4 Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage4-vk\landscape_editor_stage4_vk.png`.
- Stage-4 pixel check: both captures are `640x480`, show terrain fill plus grid overlay, 984 bright axis pixels, 238720 non-background pixels, and 214050 terrain-like pixels.
- Stage-5 validation script: `tools\verify_landscape_stage5.py`.
- Stage-5 D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage5-d3d12\landscape_editor_stage5_d3d12.png`.
- Stage-5 Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage5-vk\landscape_editor_stage5_vk.png`.
- Stage-5 pixel check: D3D12 `640x480`, 6 unique colors, 238720 non-background pixels, 984 bright axis pixels, 214050 terrain-like pixels; Vulkan `640x480`, 5 unique colors, 238720 non-background pixels, 984 bright axis pixels, 214050 terrain-like pixels.
- D3D12 pixel check passed for the grid center axes: `row_bright=513`, `col_bright=385`.
- Smoke runs succeeded on:
  - D3D12: `build\Win64-vs18\smoke-landscape-editor-d3d12\landscape_editor_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-landscape-editor-vk\landscape_editor_vk.png`
  - D3D11: `build\Win64-vs18\smoke-landscape-editor-d3d11\landscape_editor_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-landscape-editor-gl\landscape_editor_gl.png`

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

### Rendering Pipeline

- Keep two rendering pipelines available:
  - `ForwardDebugPipeline` for early terrain validation, debug drawing, wireframe, LOD overlays, and simple fallback rendering.
  - `DeferredPipeline` for the long-term opaque terrain path, GBuffer, lighting, decals, shadows, and post-processing.
- Terrain rendering code must not be hard-bound to one pipeline.
- Terrain systems should expose pass-oriented entry points such as depth, GBuffer, forward debug, and shadow rendering.
- Pipeline state objects should be cached and reused instead of rebuilt during runtime pipeline switching.
- Runtime pipeline switching should change the active pipeline and reuse cached PSOs where possible.

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

- Done: Build Diligent Engine locally with VS 18 Community.
- Done: Run `Tutorial01_HelloTriangle` with Vulkan.
- Done: Run `Tutorial01_HelloTriangle` with D3D12.
- Done: Confirm AMD RX 7900 XT behaves correctly on D3D12.
- Done: Also smoke-tested D3D11 and OpenGL.

### Phase 1: Create Landscape Prototype App

- Done: Add a new sample or app named `LandscapeEditor`.
- Keep it separate from upstream Diligent samples where possible.
- Done: Open a window and render through `ForwardDebugPipeline`.
- Done: Replace the triangle with a flat debug grid.
- Done: Add camera movement, `RenderView`, and `FrameResources`.
- Done: Introduce `ForwardRenderer`, `RenderQueue`, and project `PSOCache`.
- Done: Add the first CPU-generated flat terrain patch.
- Next: Add sun light and the first CSM shadow path.

### Phase 1.5: Complete Forward Pipeline

- Done: Add `ForwardRenderer`, `RenderQueue`, and project `PSOCache`.
- Done: Move the debug grid to world space through camera constants.
- Done: Add a CPU-generated flat terrain patch in `ForwardOpaque`.
- Done: Add sun light with four-cascade shadow maps.
- Next: Add procedural sky, transparent queue, tone mapping, and ImGui diagnostics.

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
| BUG-002 | Closed | Medium | Validation | Diligent Engine has been built in `E:\Landscape` using VS 18 Community CMake. | Reopen only if future full builds fail. |
| BUG-003 | Closed | Medium | Validation | Diligent sample runtime was verified on AMD RX 7900 XT through D3D11, D3D12, Vulkan, and OpenGL smoke captures. | Reopen only if `LandscapeEditor` fails on these backends. |
| BUG-004 | Closed | Medium | Architecture | The first milestone location is decided: root-owned `LandscapeEditor/`, not a Diligent submodule. | Reopen only if the project is moved to a separate repository. |
| BUG-005 | Open | Low | Git tooling | GitHub CLI is not installed. GitHub auth currently uses Git Credential Manager over HTTPS. | Optional: install GitHub CLI later if repository automation becomes frequent. |
| BUG-006 | Open | Low | Git tooling | SSH push is not configured because no GitHub SSH key exists on this machine. | HTTPS push works; configure SSH only if needed. |
| BUG-007 | Open | Medium | Rendering architecture | PSO cache design is not finalized. Pipeline switching should not rebuild PSOs during frame rendering. | Design PSO cache keys and validate Diligent PSO creation/reuse behavior. |
| BUG-008 | Open | Medium | Build environment | VS2022 BuildTools is missing ATL (`atlbase.h`), causing D3D11/D3D12 support to be disabled and `Win32FileSystem.cpp` to fail during build. | Use VS 18 Community for now, or install the ATL/MFC component into VS2022 BuildTools later. |
| BUG-009 | Open | Medium | Forward pipeline | Complete forward renderer is not implemented yet; current renderer has camera-driven world-space debug rendering, renderer orchestration, render queue submission, PSO cache warm-up, an opaque terrain patch, sun light constants, and four shadow cascades, but no sky, transparent queue, or postprocess pass. | Execute the remaining complete forward pipeline plan in staged commits. |

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

### Decision 004: Support Forward Debug and Deferred Pipelines

Support two render pipelines instead of locking the project to only forward or only deferred rendering.

The first prototype should use a simple forward/debug path because it is easier to bring up and easier to debug. The long-term terrain path should be deferred because large opaque terrain, layered materials, decals, shadows, screen-space effects, and post-processing fit naturally into a GBuffer-based pipeline.

Terrain rendering should be pass-oriented:

- `DrawDepth()`
- `DrawGBuffer()`
- `DrawForwardDebug()`
- `DrawShadow()`

This keeps the terrain system independent of the selected pipeline.

### Decision 005: Cache PSOs for Runtime Pipeline Switching

Pipeline state objects should be created through a cache layer.

The cache key should include at least:

- Graphics backend.
- Pipeline type.
- Render pass / framebuffer format.
- Shader variant.
- Material variant.
- Vertex layout.
- Rasterizer state.
- Depth and stencil state.
- Blend state.

Switching between forward debug and deferred rendering should select a different active pipeline and retrieve PSOs from the cache. It should not compile shaders or create PSOs during normal frame rendering.

### Decision 006: Keep Landscape Code Out of Submodules

Custom Landscape project code should live in root-owned project folders such as `LandscapeEditor/`.

Reasoning:

- `DiligentSamples` is a Git submodule.
- New files inside a submodule are not committed as normal files in the root `MatouSakura/Landscape` repository.
- Keeping project code outside submodules makes clone, push, review, and future migration simpler.
- Root `CMakeLists.txt` can add `LandscapeEditor` after `DiligentSamples`, reusing `add_sample_app()` without modifying the submodule.

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

### Build With Visual Studio 18 CMake

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build\Win64-vs18 -G "Visual Studio 18 2026" -A x64 -DPython3_EXECUTABLE="C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target Tutorial01_HelloTriangle --parallel
```

### Smoke Run Backends

```powershell
cd E:\Landscape\build\Win64-vs18\DiligentSamples\Tutorials\Tutorial01_HelloTriangle\Release
.\Tutorial01_HelloTriangle.exe --mode d3d12 --adapters_dialog 0 --golden_image_mode capture --capture_path E:\Landscape\build\Win64-vs18\smoke-d3d12 --capture_name hello_d3d12 --capture_format png --capture_alpha 0 --show_ui 0 -w 640 -h 480
```

### Build LandscapeEditor

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build\Win64-vs18 -G "Visual Studio 18 2026" -A x64 -DPython3_EXECUTABLE="C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

### Smoke Run LandscapeEditor

```powershell
cd E:\Landscape\build\Win64-vs18\LandscapeEditor\Release
.\LandscapeEditor.exe --mode d3d12 --adapters_dialog 0 --golden_image_mode capture --capture_path E:\Landscape\build\Win64-vs18\smoke-landscape-editor-d3d12 --capture_name landscape_editor_d3d12 --capture_format png --capture_alpha 0 --show_ui 0 -w 640 -h 480
```

## Next Immediate Steps

1. Add procedural sky rendering.
2. Add a transparent queue with alpha-blend PSO and distance sorting.
3. Add a tone mapping/gamma postprocess pass.
4. Re-run D3D12, Vulkan, D3D11, and OpenGL smoke captures.
5. Start defining the heightmap terrain data model after the forward pipeline is complete.

## Notes

- Keep upstream Diligent code as clean as possible.
- Prefer adding Landscape-specific code in isolated folders.
- Avoid editing Diligent internals unless the terrain system truly requires it.
- Keep unresolved issues in this document until they are verified as fixed.
