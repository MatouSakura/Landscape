# Landscape Project Status

Last updated: 2026-06-23

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
- Added the sky, transparent, and postprocess implementation plan:
  `docs/superpowers/plans/2026-06-22-sky-transparent-postprocess.md`
- Added the forward pipeline hardening plan:
  `docs/superpowers/plans/2026-06-22-forward-pipeline-hardening.md`
- Added the OpenGL postprocess BUG-010 fix record:
  `docs/superpowers/plans/2026-06-23-bug010-opengl-postprocess.md`
- Added the heightfield terrain patch implementation plan:
  `docs/superpowers/plans/2026-06-23-heightfield-terrain-patch.md`
- Added the CPU quadtree LOD selection implementation plan:
  `docs/superpowers/plans/2026-06-23-cpu-quadtree-lod-selection.md`
- Added the selected leaf terrain render items implementation plan:
  `docs/superpowers/plans/2026-06-23-selected-leaf-terrain-render-items.md`
- Added the packed tile mesh cache implementation plan:
  `docs/superpowers/plans/2026-06-23-packed-tile-mesh-cache.md`
- Added the terrain tile skirts implementation plan:
  `docs/superpowers/plans/2026-06-23-terrain-tile-skirts.md`
- Added the LOD tile sampling implementation plan:
  `docs/superpowers/plans/2026-06-23-lod-tile-sampling.md`
- Added the LOD transition diagnostics implementation plan:
  `docs/superpowers/plans/2026-06-23-lod-transition-diagnostics.md`
- Added the terrain frustum culling implementation plan:
  `docs/superpowers/plans/2026-06-23-terrain-frustum-culling.md`
- Added the scripted camera smoke presets implementation plan:
  `docs/superpowers/plans/2026-06-23-scripted-camera-smoke.md`
- Added the UE-style component LOD policy implementation plan:
  `docs/superpowers/plans/2026-06-23-ue-style-component-lod-policy.md`
- Added the LOD stitching seam plan implementation record:
  `docs/superpowers/plans/2026-06-23-lod-stitching-seam-plan.md`
- Added the LOD index stitching v1 implementation record:
  `docs/superpowers/plans/2026-06-23-lod-index-stitching-v1.md`
- Added the LOD index stitching corner hardening implementation record:
  `docs/superpowers/plans/2026-06-23-lod-index-stitching-corner-hardening.md`
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
- Renders a procedural sky pass after opaque terrain using far-depth testing.
- Submits and renders a transparent alpha-blended test quad through a transparent queue.
- Renders through a scene-color target and postprocess pass boundary.
- Uses shader tone mapping/gamma on D3D12, Vulkan, D3D11, and OpenGL.
- Uses backend-specific shader source for postprocess: HLSL for D3D12/Vulkan/D3D11 and GLSL for OpenGL.
- Renders a deterministic procedural heightfield terrain patch with CPU-generated normals, UVs, and height statistics.
- Builds a complete CPU quadtree over the current terrain extent with stable node indices and contiguous direct child nodes.
- Selects quadtree LOD leaves every frame from camera XZ distance.
- Draws selected quadtree leaves through a debug line overlay with backend-specific GLSL for OpenGL.
- Exposes quadtree node count, selected leaf count, max depth, max selected LOD, and overlay toggle in ImGui.
- Converts selected leaf terrain render items into opaque queue items with node bounds and heightfield cell ranges.
- Uses the selected leaf terrain render items for both forward opaque terrain and shadow depth rendering.
- Tracks terrain render item count, rendered cell count, and forward/shadow terrain draw-call counts in ImGui.
- Builds a packed tile mesh cache for every quadtree node during terrain renderer initialization.
- Draws selected terrain leaves with one indexed draw per tile mesh using `BaseVertex`, `FirstIndexLocation`, and `NumIndices`.
- Tracks packed tile mesh count, packed vertex count, and packed index count in ImGui.
- Adds terrain tile skirts to each packed tile mesh as a first-pass LOD crack-hiding policy.
- Tracks terrain skirt enabled state, skirt depth, skirt vertices, and skirt indices in ImGui.
- Adds LOD tile sampling so coarser quadtree levels use larger heightfield sample steps in their packed tile meshes.
- Tracks rendered mesh cell count separately from covered terrain cell count, plus min/max LOD sample step in ImGui.
- Adds LOD transition diagnostics for quadtree debug overlay: selected leaf bounds, cyan skirt edges, and mixed-level transition edges.
- Tracks debug leaf bound lines, skirt edge count, LOD transition edge count, and debug line vertex count in ImGui.
- Adds CPU frustum culling for selected terrain leaves before render queue submission and debug overlay generation.
- Tracks terrain frustum culling enabled state, candidate leaf count, visible leaf count, and culled leaf count in ImGui.
- Adds scripted camera smoke presets through `--landscape_camera_preset default|mixed_lod|off_frustum`.
- Shows the active scripted camera preset in the ImGui diagnostics panel.
- Adds a UE-style component LOD policy over the quadtree so near leaves can remain fine while far leaves stay larger.
- Exposes `LOD distance scale`, `Max terrain LOD`, selected LOD range, and selected component world-size range in ImGui.
- Builds a neighbor-aware LOD stitching seam plan from the visible selected leaves every frame.
- Draws mixed-level transition overlay lines from `TerrainLODStitching` seam records instead of ad hoc debug-only pair checks.
- Exposes LOD stitching seam count, max LOD delta, max stitch ratio, and total seam length in ImGui.
- Converts LOD stitching seam records into dynamic per-frame stitched index regions for fine tiles bordering coarser selected leaves.
- Draws stitched fine-edge terrain through a dynamic index buffer while preserving the existing packed immutable terrain vertex buffer.
- Uses stitched index regions for both forward terrain and shadow depth terrain draws.
- Exposes LOD index stitched node count, stitched edge count, generated index count, and max stitch ratio in ImGui.
- Clips stitched edge bands when two stitched terrain edges meet at a corner, then fills each stitched corner once with corner patch indices.
- Exposes LOD index stitched corner count, corner patch index count, and an `Enable LOD index stitching` comparison toggle in ImGui.
- Allows stitched-index terrain rendering to be disabled at runtime while keeping skirts and seam diagnostics visible for comparison.
- Uses OpenGL-specific GLSL shader paths for postprocess, sky, and heightfield terrain where backend translation or depth behavior needs explicit handling.
- Binds render targets before clear operations so OpenGL smoke runs without clear-target errors.
- Draws sky before opaque terrain on OpenGL only; D3D12, Vulkan, and D3D11 keep the normal opaque-then-sky pass order.
- Keeps external heightmap loading, full cascade selection/stabilization, GPU culling, indirect draw, and production terrain materials deferred until the terrain data model is expanded.

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
- Stage-6 validation script: `tools\verify_landscape_stage6.py`.
- Stage-6 D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-d3d12\landscape_editor_stage6_d3d12.png`.
- Stage-6 Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-vk\landscape_editor_stage6_vk.png`.
- Stage-6 D3D11 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-d3d11\landscape_editor_stage6_d3d11.png`.
- Stage-6 OpenGL smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-gl\landscape_editor_stage6_gl.png`.
- Stage-6 pixel check: all four captures are `640x480`, have visible sky/grid/terrain/transparent content, and have non-background coverage of 303360 pixels.
- Final forward completion validation script: `tools\verify_landscape_forward_completion.py`.
- Final validation confirmed Stage-2 through Stage-6 validation scripts still pass.
- Final PSO stability check: renderer `Render()` methods do not call `PSOCache::GetOrCreate` or `CreateGraphicsPipelineState`; PSO creation is kept in initialization-time cache paths.
- Final D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-final-d3d12\landscape_editor_final_d3d12.png`.
- Final Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-final-vk\landscape_editor_final_vk.png`.
- Final D3D11 smoke capture: `build\Win64-vs18\smoke-landscape-editor-final-d3d11\landscape_editor_final_d3d11.png`.
- Final OpenGL smoke capture: `build\Win64-vs18\smoke-landscape-editor-final-gl\landscape_editor_final_gl.png`.
- Final pixel check: all four captures are `640x480`, have visible sky/grid/terrain/transparent content, and have non-background coverage of 303360 pixels.
- D3D12 pixel check passed for the grid center axes: `row_bright=513`, `col_bright=385`.
- Smoke runs succeeded on:
  - D3D12: `build\Win64-vs18\smoke-landscape-editor-d3d12\landscape_editor_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-landscape-editor-vk\landscape_editor_vk.png`
  - D3D11: `build\Win64-vs18\smoke-landscape-editor-d3d11\landscape_editor_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-landscape-editor-gl\landscape_editor_gl.png`

Heightfield terrain patch validation completed on 2026-06-23:

- Build: `LandscapeEditor` Release target succeeded with VS18 CMake.
- Static validation:
  - `tools\verify_landscape_stage4.py`
  - `tools\verify_landscape_stage5.py`
  - `tools\verify_landscape_stage6.py`
  - `tools\verify_landscape_forward_completion.py`
  - `tools\verify_landscape_heightfield.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-heightfield-d3d12\landscape_heightfield_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-heightfield-vk\landscape_heightfield_vk.png`
  - D3D11: `build\Win64-vs18\smoke-heightfield-d3d11\landscape_heightfield_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-heightfield-gl\landscape_heightfield_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include non-background coverage across the full frame.

CPU quadtree LOD selection validation completed on 2026-06-23:

- Build: `LandscapeEditor` Release target succeeded with VS18 CMake.
- Static validation:
  - `tools\verify_landscape_stage4.py`
  - `tools\verify_landscape_stage5.py`
  - `tools\verify_landscape_stage6.py`
  - `tools\verify_landscape_forward_completion.py`
  - `tools\verify_landscape_heightfield.py`
  - `tools\verify_landscape_quadtree_lod.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-quadtree-d3d12\landscape_quadtree_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-quadtree-vk\landscape_quadtree_vk.png`
  - D3D11: `build\Win64-vs18\smoke-quadtree-d3d11\landscape_quadtree_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-quadtree-gl\landscape_quadtree_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include visible quadtree overlay line pixels.
- Visual check: D3D12 capture shows selected quadtree leaf rectangles over the heightfield terrain.
- Implementation note: the quadtree builder inserts each node's four direct children contiguously before recursively subdividing, so `FirstChildIndex + 0..3` is valid for SW, SE, NW, and NE traversal.

Selected leaf terrain render items validation completed on 2026-06-23:

- Build: `LandscapeEditor` Release target succeeded with VS18 CMake.
- Static validation:
  - `tools\verify_landscape_stage4.py`
  - `tools\verify_landscape_stage5.py`
  - `tools\verify_landscape_stage6.py`
  - `tools\verify_landscape_forward_completion.py`
  - `tools\verify_landscape_heightfield.py`
  - `tools\verify_landscape_quadtree_lod.py`
  - `tools\verify_landscape_selected_leaf_render_items.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-selected-leaves-d3d12\landscape_selected_leaves_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-selected-leaves-vk\landscape_selected_leaves_vk.png`
  - D3D11: `build\Win64-vs18\smoke-selected-leaves-d3d11\landscape_selected_leaves_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-selected-leaves-gl\landscape_selected_leaves_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include visible quadtree overlay line pixels.
- Visual check: D3D12 capture shows the heightfield terrain still covers the patch after switching opaque terrain submission from one full-patch item to selected leaf render items.
- Implementation note: this stage uses row-based indexed draws with `FirstIndexLocation` over the existing heightfield index buffer. It establishes render item granularity before introducing per-node mesh buffers or a reusable tile grid.

Packed tile mesh cache validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-packed-tiles-d3d12\landscape_packed_tiles_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-packed-tiles-vk\landscape_packed_tiles_vk.png`
  - D3D11: `build\Win64-vs18\smoke-packed-tiles-d3d11\landscape_packed_tiles_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-packed-tiles-gl\landscape_packed_tiles_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include visible quadtree overlay line pixels.
- Visual check: D3D12 capture shows the heightfield terrain remains continuous after switching from row-based region drawing to packed per-node tile mesh ranges.
- Implementation note: each quadtree node now has a prebuilt tile mesh range in the shared immutable terrain vertex and index buffers. Selected leaf render items draw those ranges with `BaseVertex`, `FirstIndexLocation`, and `NumIndices`.

Terrain tile skirts validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-tile-skirts-d3d12\landscape_tile_skirts_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-tile-skirts-vk\landscape_tile_skirts_vk.png`
  - D3D11: `build\Win64-vs18\smoke-tile-skirts-d3d11\landscape_tile_skirts_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-tile-skirts-gl\landscape_tile_skirts_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include visible quadtree overlay line pixels.
- Visual check: D3D12 capture shows the selected packed tiles still render continuously with terrain tile skirts enabled.
- Implementation note: skirt indices are appended after each tile's main surface indices so the renderer can draw either main-only or main-plus-skirt geometry through the `Enable terrain skirts` debug toggle.

LOD tile sampling validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-lod-sampling-d3d12\landscape_lod_sampling_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-sampling-vk\landscape_lod_sampling_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-sampling-d3d11\landscape_lod_sampling_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-sampling-gl\landscape_lod_sampling_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include visible quadtree overlay line pixels.
- Visual check: D3D12 capture shows terrain remains continuous after level-dependent tile mesh sampling was enabled.
- Implementation note: default quadtree depth `4` maps level `0..4` to LOD sample steps `16, 8, 4, 2, 1`; tile coordinate arrays always include both tile boundaries so each mesh still covers its selected leaf bounds.

LOD transition diagnostics validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-lod-transitions-d3d12\landscape_lod_transitions_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-transitions-vk\landscape_lod_transitions_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-transitions-d3d11\landscape_lod_transitions_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-transitions-gl\landscape_lod_transitions_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include increased overlay line pixels from the skirt edge overlay.
- Visual check: D3D12 capture shows cyan skirt edge diagnostics in addition to the existing selected leaf bounds.
- Implementation note: transition edge detection is CPU-side and compares selected leaf bounds pairwise for shared edge overlap with different LOD levels. The current default capture has no red transition pixels from its camera position, but the overlay path and counters are wired for mixed-level selections.

Terrain frustum culling validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-frustum-culling-d3d12\landscape_frustum_culling_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-frustum-culling-vk\landscape_frustum_culling_vk.png`
  - D3D11: `build\Win64-vs18\smoke-frustum-culling-d3d11\landscape_frustum_culling_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-frustum-culling-gl\landscape_frustum_culling_gl.png`
- Pixel check: all four captures are `640x480`, have visible terrain/grid/transparent/sky content, and include visible debug overlay line pixels.
- Visual check: D3D12 capture shows terrain, grid, transparent quad, sky, and quadtree/skirt overlays still visible with terrain frustum culling enabled.
- Implementation note: distance-based quadtree selection remains the candidate set. `ForwardRenderer` extracts a `ViewFrustum` from `RenderView.ViewProj`, builds per-node AABBs from XZ bounds and terrain height range, and submits only visible leaves to terrain rendering and debug overlay.

Scripted camera smoke presets validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12 default: `build\Win64-vs18\smoke-scripted-camera-default-d3d12\landscape_scripted_default_d3d12.png`
  - Vulkan default: `build\Win64-vs18\smoke-scripted-camera-default-vk\landscape_scripted_default_vk.png`
  - D3D11 default: `build\Win64-vs18\smoke-scripted-camera-default-d3d11\landscape_scripted_default_d3d11.png`
  - OpenGL default: `build\Win64-vs18\smoke-scripted-camera-default-gl\landscape_scripted_default_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-scripted-camera-mixed-lod-d3d12\landscape_scripted_mixed_lod_d3d12.png`
  - D3D12 `off_frustum`: `build\Win64-vs18\smoke-scripted-camera-off-frustum-d3d12\landscape_scripted_off_frustum_d3d12.png`
- Pixel check: all six captures are `640x480`, have visible terrain, and include cyan diagnostic overlay pixels.
- Visual check: `mixed_lod` and `off_frustum` keep side-biased terrain/overlay views for mixed-level LOD transition and off-frustum culling regression captures.
- Implementation note: `LandscapeEditor::ProcessCommandLine()` consumes `--landscape_camera_preset` after common `SampleApp` arguments are pruned. Presets are applied before `UpdateRenderView()`.

UE-style component LOD policy validation completed on 2026-06-23:

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
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-component-lod-policy-d3d12\landscape_component_lod_policy_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-component-lod-policy-vk\landscape_component_lod_policy_vk.png`
  - D3D11: `build\Win64-vs18\smoke-component-lod-policy-d3d11\landscape_component_lod_policy_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-component-lod-policy-gl\landscape_component_lod_policy_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-component-lod-policy-mixed-d3d12\landscape_component_lod_policy_mixed_d3d12.png`
- Pixel check: all five captures are `640x480`, have visible terrain, and include cyan diagnostic overlay pixels.
- Visual check: D3D12 `mixed_lod` keeps side-biased terrain visible with selected component overlay lines.
- Implementation note: `TerrainComponentLODPolicy` now owns split distance scale and max selectable LOD. The renderer exposes selected component world-size range so this quadtree leaf system can be tuned like a UE-style near/far component LOD policy.

LOD stitching seam plan validation completed on 2026-06-23:

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
  - `tools\verify_landscape_lod_stitching.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-lod-stitching-d3d12\landscape_lod_stitching_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-stitching-vk\landscape_lod_stitching_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-stitching-d3d11\landscape_lod_stitching_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-stitching-gl\landscape_lod_stitching_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-lod-stitching-mixed-d3d12\landscape_lod_stitching_mixed_d3d12.png`
- Pixel check: all five captures are `640x480`, have full-frame non-dark coverage, and keep terrain visible.
- Mixed LOD pixel check: `mixed_lod` has 5536 cyan diagnostic pixels and 349 orange seam-transition pixels after postprocess tone mapping.
- Visual check: D3D12 `mixed_lod` shows visible selected leaf and seam overlay lines on the side-biased terrain view.
- Implementation note: this stage creates formal neighbor seam metadata for mixed-LOD selected leaves. It does not yet rebuild tile edge indices or apply shader morphing; those are the next crack-fixing steps.

LOD index stitching v1 validation completed on 2026-06-23:

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
  - `tools\verify_landscape_lod_stitching.py`
  - `tools\verify_landscape_lod_index_stitching.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-lod-index-stitching-d3d12\landscape_lod_index_stitching_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-index-stitching-vk\landscape_lod_index_stitching_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-index-stitching-d3d11\landscape_lod_index_stitching_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-index-stitching-gl\landscape_lod_index_stitching_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-lod-index-stitching-mixed-d3d12\landscape_lod_index_stitching_mixed_d3d12.png`
- Pixel check: all five captures are `640x480`, have full-frame non-dark coverage, and keep terrain visible.
- Mixed LOD pixel check: `mixed_lod` has 5398 cyan diagnostic pixels and 349 orange seam-transition pixels after postprocess tone mapping.
- Visual check: D3D12 `mixed_lod` remains side-biased and keeps terrain plus selected leaf and seam overlay lines visible while stitched index regions are active.
- Implementation note: this is the first real index-stitching draw path. It replaces fine tile edge bands with dynamic fan indices for fine/coarse seams while leaving corner overlap hardening and optional shader morphing for later.

LOD index stitching corner hardening validation completed on 2026-06-23:

- Build: `LandscapeEditor` Release target succeeded with VS18 CMake.
- Static validation:
  - `tools\verify_landscape_lod_index_stitching_corners.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-lod-corner-hardening-d3d12\landscape_lod_corner_hardening_d3d12.png`
  - Vulkan: `build\Win64-vs18\smoke-lod-corner-hardening-vk\landscape_lod_corner_hardening_vk.png`
  - D3D11: `build\Win64-vs18\smoke-lod-corner-hardening-d3d11\landscape_lod_corner_hardening_d3d11.png`
  - OpenGL: `build\Win64-vs18\smoke-lod-corner-hardening-gl\landscape_lod_corner_hardening_gl.png`
  - D3D12 `mixed_lod`: `build\Win64-vs18\smoke-lod-corner-hardening-mixed-d3d12\landscape_lod_corner_hardening_mixed_d3d12.png`
- Pixel check:
  - D3D12: `640x480`, `non_dark=307200`, `ground_like=236192`, `grid_like=222801`, `cyan_like=3894`, `orange_like=1603`, `std_sum=38.56`.
  - Vulkan: `640x480`, `non_dark=307200`, `ground_like=236192`, `grid_like=222801`, `cyan_like=3894`, `orange_like=1603`, `std_sum=38.56`.
  - D3D11: `640x480`, `non_dark=307200`, `ground_like=236192`, `grid_like=222801`, `cyan_like=3894`, `orange_like=1603`, `std_sum=38.56`.
  - OpenGL: `640x480`, `non_dark=307200`, `ground_like=194231`, `grid_like=289532`, `cyan_like=2646`, `orange_like=772`, `std_sum=48.52`.
  - D3D12 `mixed_lod`: `640x480`, `non_dark=307200`, `ground_like=64310`, `grid_like=240730`, `cyan_like=1268`, `orange_like=455`, `std_sum=32.12`.
- Implementation note: corner-hardening removes duplicate edge-band corner coverage by clipping stitched edge bands and adding one corner patch per stitched corner. Shader morph comparison remains a later option.

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
- Rendering path: packed per-node tile mesh cache first, indirect draw later

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
- Done: Add procedural sky, transparent queue, tone mapping, and ImGui diagnostics.
- Done: Harden docs, PSO stability checks, and final four-backend smoke records.
- Complete: the first forward renderer milestone is ready for terrain data/quadtree work.

### Phase 2: Basic Heightmap Terrain

- Done: Define the first heightfield terrain data model.
- Done: Generate deterministic procedural height samples for one fixed patch.
- Done: Generate terrain vertex/index buffers from height samples.
- Done: Render a fixed-size heightfield terrain patch.
- Done: Add basic CPU normal calculation.
- Done: Add simple height/slope terrain material variation.
- Next: Add external heightmap loading for one fixed terrain patch.
- Next: Start CPU quadtree node model and LOD selection.

### Phase 3: Quadtree LOD

- Done: Split terrain into CPU quadtree nodes with stable node indices.
- Done: Select LOD based on camera XZ distance.
- Done: Add debug overlay for selected quadtree nodes and LOD levels.
- Done: Convert selected quadtree leaves into terrain render items.
- Done: Add packed per-node tile mesh cache for selected leaf render items.
- Done: Add first-pass terrain tile skirts for selected leaf render items.
- Done: Add reduced-resolution tile sampling per quadtree level.
- Done: Add LOD transition diagnostics and crack-boundary inspection.
- Done: Add CPU frustum culling once node bounds are stable.
- Done: Add camera/debug controls or scripted smoke positions for mixed-level LOD transition and off-frustum culling regression captures.
- Done: Add UE-style component LOD policy controls so near selected leaves stay fine and far selected leaves can remain larger.
- Done: Add neighbor-aware LOD stitching seam plan metadata for mixed-resolution selected leaves.
- Done: Convert seam metadata into dynamic LOD index stitching v1 for selected fine/coarse leaf neighbors.
- Done: Harden LOD index stitching corners and add a stitched-index comparison toggle.
- Later: Compare the stitched-index path against optional shader morphing.

### Phase 4: LOD Crack Fixing

- Done: Add skirt-based crack hiding as the first stable implementation.
- Done: Add mixed-density tile sampling so LOD transitions are geometrically different.
- Done: Add diagnostics for level boundaries and skirt visibility.
- Done: Build first formal LOD stitching seam plan for mixed-level leaf neighbors.
- Done: Convert seam plan into actual dynamic stitched edge indices for fine/coarse selected leaf boundaries.
- Done: Harden corner cases where two stitched edges meet and add a toggle to compare stitched-index rendering with skirt-only rendering.
- Next: Add shader morph comparison captures or start external heightmap loading.
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
| BUG-009 | Closed | Medium | Forward pipeline | The first complete forward pass chain has been implemented and hardened with PSO stability checks, final docs, and final D3D12/Vulkan/D3D11/OpenGL smoke records. | Reopen only if forward renderer smoke or validation regresses. |
| BUG-010 | Closed | Medium | OpenGL postprocess | OpenGL now uses a dedicated GLSL postprocess shader to sample the offscreen scene color. The old `CopyTexture` fallback has been removed, and all four backends use shader postprocess. | Reopen only if OpenGL postprocess smoke regresses. |

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

1. Add shader morph comparison captures for LOD transitions, or start external heightmap loading for one fixed terrain patch.
2. Keep validating stitched-index LOD transitions while moving the camera.
3. Keep OpenGL postprocess and OpenGL sky/terrain/quadtree overlay paths in the regular smoke set so `BUG-010` stays closed and GL terrain remains visible.

## Notes

- Keep upstream Diligent code as clean as possible.
- Prefer adding Landscape-specific code in isolated folders.
- Avoid editing Diligent internals unless the terrain system truly requires it.
- Keep unresolved issues in this document until they are verified as fixed.
