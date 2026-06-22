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

## Follow-Up

The next slice will add sun light and a four-cascade shadow-map path for the terrain patch.
