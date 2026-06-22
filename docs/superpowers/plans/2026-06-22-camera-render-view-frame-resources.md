# Camera, RenderView, and FrameResources Plan

Date: 2026-06-22

## Goal

Move the current flat debug grid from clip-space rendering to camera-driven world-space rendering.

This slice adds the first runtime camera, `RenderView`, and frame constant buffer path without yet introducing the full `ForwardRenderer`.

## Implementation

- Add `RenderView` to describe view/projection data, camera position, viewport, clip planes, and output formats.
- Add `FrameResources` to own a dynamic camera constant buffer and update it every frame.
- Add `FirstPersonCamera` ownership to `LandscapeEditor`.
- Change `ForwardDebugPipeline::Render()` to consume `RenderView` and `FrameResources`.
- Change the debug grid shader to transform world-space lines through `ViewProj`.
- Add a lightweight validation script that checks the expected stage-2 renderer structure before code is considered complete.

## Verification

- Run the stage-2 validation script before implementation and confirm it fails because the new structures are absent.
- Build `LandscapeEditor` Release.
- Run a D3D12 smoke capture.
- Run the stage-2 validation script after implementation and confirm it passes.

## Follow-Up

The next slice will introduce `ForwardRenderer`, `RenderQueue`, and `PSOCache` so the grid is submitted through a normal renderer orchestration path.
