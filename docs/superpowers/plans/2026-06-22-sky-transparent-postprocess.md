# Sky, Transparent, and Postprocess Plan

Date: 2026-06-22

## Goal

Complete the first forward-renderer auxiliary pass set for `LandscapeEditor`.

This slice adds a procedural sky pass, a simple alpha-blended transparent queue, and a postprocess pass boundary. The goal is not final visual quality; it is to establish stable renderer ordering, cached PSOs, queue plumbing, and backend smoke coverage for the complete forward pass chain.

## Implementation

- Add `SkyRenderer` with a cached full-screen procedural sky PSO.
- Render sky after opaque terrain with depth testing so it fills only untouched far-depth pixels.
- Extend `RenderQueue` with a transparent queue and sorted transparent item support.
- Add `TransparentRenderer` with a cached alpha-blend PSO and a small world-space test quad.
- Add `PostProcessRenderer` as the first postprocess pass boundary using a cached full-screen gamma/tone-map PSO.
- Update `ForwardRenderer` order to `ShadowPass -> ForwardOpaque -> Sky -> Transparent -> Debug -> PostProcess`.
- Extend renderer stats and ImGui diagnostics for sky, transparent, and postprocess pass activity.

## Verification

- Add a stage-6 validation script.
- Build `LandscapeEditor` Release.
- Run D3D12, Vulkan, D3D11, and OpenGL smoke captures.
- Confirm captures show terrain, grid, sky/background contribution, and transparent overlay.
- Confirm validation scripts for stages 2 through 6 pass.

## Result

Completed on 2026-06-22.

- Added `SkyRenderer` with a cached full-screen procedural sky PSO.
- Added `TransparentRenderer` with a cached alpha-blend PSO and a world-space test quad submitted through the transparent queue.
- Added `PostProcessRenderer` with a scene-color render target and cached full-screen tone-map/gamma PSO.
- Updated `ForwardRenderer` pass order to `ShadowPass -> ForwardOpaque -> Sky -> Transparent -> Debug -> PostProcess`.
- Extended renderer stats and ImGui diagnostics for sky, transparent, and postprocess pass counts.
- Added `tools\verify_landscape_stage6.py`.

Validation:

- `tools\verify_landscape_stage6.py`: passed.
- Build: `cmake --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel`: passed.
- D3D12 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-d3d12\landscape_editor_stage6_d3d12.png`.
- Vulkan smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-vk\landscape_editor_stage6_vk.png`.
- D3D11 smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-d3d11\landscape_editor_stage6_d3d11.png`.
- OpenGL smoke capture: `build\Win64-vs18\smoke-landscape-editor-stage6-gl\landscape_editor_stage6_gl.png`.
- Pixel check:
  - D3D12: `640x480`, 25 unique colors, 303360 non-background pixels, 103934 bright axis pixels, 214050 terrain-like pixels, 93150 sky-like pixels.
  - Vulkan: `640x480`, 22 unique colors, 303360 non-background pixels, 103934 bright axis pixels, 214050 terrain-like pixels, 93150 sky-like pixels.
  - D3D11: `640x480`, 25 unique colors, 303360 non-background pixels, 103934 bright axis pixels, 214050 terrain-like pixels, 93150 sky-like pixels.
  - OpenGL: `640x480`, 169 unique colors, 303360 non-background pixels, 63690 bright axis pixels, visible sky/grid/transparent quad.

Debug note:

- D3D12, Vulkan, and D3D11 use the shader tone-map/gamma postprocess path.
- OpenGL hit an access violation when the postprocess shader sampled the offscreen scene color during golden-image capture. The OpenGL backend now uses a `CopyTexture` fallback for the postprocess boundary while keeping the offscreen scene-color architecture. This is tracked as a follow-up compatibility issue.

## Follow-Up

The next slice will harden documentation and final verification, including PSO creation-count stability and final four-backend smoke records.
