# Sun Light and CSM Shadow Plan

Date: 2026-06-22

## Goal

Add the first sun-light and cascaded-shadow path to the forward renderer.

This slice should create four shadow cascades, render the terrain patch into shadow depth maps, update light constants, and let the forward terrain shader sample the first cascade. It establishes the CSM architecture; later slices can improve cascade selection, stabilization, filtering quality, and terrain LOD integration.

## Implementation

- Add light constants to `FrameResources`: sun direction, colors, shadow matrices, cascade splits, and shadow parameters.
- Add `ShadowRenderer` with four depth textures, DSV/SRV views, shadow constants, and a terrain shadow pass.
- Add `TerrainPatchRenderer::RenderShadow()` using the existing terrain vertex/index buffers and a shadow-depth PSO.
- Update the terrain forward shader to use sun lighting and sample the first CSM shadow map.
- Run `ShadowPass` before `ForwardOpaque` in `ForwardRenderer`.
- Expose shadow cascade count and shadow map size in renderer stats/ImGui.

## Verification

- Add a stage-5 validation script and confirm it fails before implementation.
- Build `LandscapeEditor` Release.
- Run D3D12 and Vulkan smoke captures.
- Confirm captures still show terrain fill and grid overlay.
- Confirm validation scripts for stages 2 through 5 pass.

## Follow-Up

The next slice will add procedural sky, transparent rendering, and a simple postprocess pass.
