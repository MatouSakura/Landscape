# Forward Opaque Terrain Patch Plan

Date: 2026-06-22

## Goal

Add the first CPU-generated flat terrain patch and render it through the forward opaque queue.

This keeps the current debug grid but makes the renderer draw an actual indexed terrain surface before moving on to shadows.

## Implementation

- Add `TerrainPatchRenderer` with CPU-generated vertex and index buffers.
- Generate a flat grid patch on the XZ plane with normals and a simple forward material color.
- Add a terrain opaque PSO through the project `PSOCache`.
- Extend `RenderQueue` with `Opaque` terrain submission and counters.
- Extend `ForwardRenderer` to draw opaque terrain before debug grid.
- Extend debug UI stats with opaque item count and triangle count.

## Verification

- Add a stage-4 validation script and confirm it fails before implementation.
- Build `LandscapeEditor` Release.
- Run D3D12 and Vulkan smoke captures.
- Confirm captures show terrain fill plus grid overlay.
- Confirm validation scripts for stages 2, 3, and 4 pass.

## Result

- Red check: `tools\verify_landscape_stage4.py` initially failed because `TerrainPatchRenderer`, opaque queue submission, and terrain buffers were absent.
- Green check: `tools\verify_landscape_stage4.py` now passes.
- Build: `LandscapeEditor` Release builds with VS 18 CMake.
- Runtime D3D12: `build\Win64-vs18\smoke-landscape-editor-stage4-d3d12\landscape_editor_stage4_d3d12.png`.
- Runtime Vulkan: `build\Win64-vs18\smoke-landscape-editor-stage4-vk\landscape_editor_stage4_vk.png`.
- Pixel check: both captures are `640x480`, have terrain fill plus grid overlay, 984 bright axis pixels, 238720 non-background pixels, and 214050 terrain-like pixels.
- Architecture result: `TerrainPatchRenderer` creates immutable vertex/index buffers, retrieves its PSO from `PSOCache`, and is drawn from `ForwardRenderer` through the `Opaque` queue before debug rendering.

## Follow-Up

The next slice will add sun light and a four-cascade shadow-map path for the terrain patch.
