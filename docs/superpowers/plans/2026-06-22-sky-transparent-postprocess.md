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

## Follow-Up

The next slice will harden documentation and final verification, including PSO creation-count stability and final four-backend smoke records.
