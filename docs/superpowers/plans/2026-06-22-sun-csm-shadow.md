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

## Result

Completed on 2026-06-22.

- Added `FrameResources::LightConstants` for sun direction, sun color, ambient color, four shadow matrices, cascade splits, and shadow parameters.
- Added `ShadowRenderer` with four D16 shadow depth maps, DSV/SRV views, per-cascade light matrices, and shadow pass constants.
- Added `TerrainPatchRenderer::RenderShadow()` and a cached terrain shadow-depth PSO.
- Updated the terrain forward shader to use sun lighting and sample the first cascade shadow map.
- Updated `ForwardRenderer` ordering to run `ShadowPass -> ForwardOpaque -> Debug`.
- Exposed shadow cascade count and shadow map size through renderer stats and ImGui.

Validation:

- `tools\verify_landscape_stage5.py`: passed.
- Build: `cmake --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel`: passed.
- D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage5-d3d12\landscape_editor_stage5_d3d12.png`.
- Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage5-vk\landscape_editor_stage5_vk.png`.
- Pixel check:
  - D3D12: `640x480`, 6 unique colors, 238720 non-background pixels, 984 bright axis pixels, 214050 terrain-like pixels.
  - Vulkan: `640x480`, 5 unique colors, 238720 non-background pixels, 984 bright axis pixels, 214050 terrain-like pixels.

Debug note:

- A runtime access violation was traced to the terrain pixel shader using `LightConstants` and `g_ShadowMap0` without declaring them in the pixel shader source. The declarations were initially placed in the vertex shader source string. Moving them into `TerrainPS` fixed D3D12 capture and kept Vulkan smoke green.

## Follow-Up

The next slice will add procedural sky, transparent rendering, and a simple postprocess pass.
