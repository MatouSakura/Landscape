# ForwardRenderer, RenderQueue, and PSOCache Plan

Date: 2026-06-22

## Goal

Introduce the first forward renderer orchestration layer without changing the visible output.

The current world-space debug grid should be submitted through a `RenderQueue`, rendered by `ForwardRenderer`, and use a project-level `PSOCache` so normal frame rendering does not create PSOs.

## Implementation

- Add `RenderQueue` with initial `Debug` submission support and queue counters.
- Add `PSOCache` with deterministic string keys, `GetOrCreate()`, PSO count, and creation count.
- Add `ForwardRenderer` that owns `ForwardDebugPipeline`, `RenderQueue`, and `PSOCache`.
- Change `LandscapeEditor` to own `ForwardRenderer` instead of calling `ForwardDebugPipeline` directly.
- Warm up the debug grid PSO during renderer initialization.
- Expose renderer stats to ImGui: debug item count, PSO count, and PSO creation count.

## Verification

- Add a stage-3 validation script and confirm it fails before implementation.
- Build `LandscapeEditor` Release.
- Run D3D12 and Vulkan smoke captures.
- Confirm the D3D12 capture still shows the world-space grid.
- Confirm the validation script passes after implementation.

## Result

- Red check: `tools\verify_landscape_stage3.py` initially failed because `ForwardRenderer`, `RenderQueue`, and `PSOCache` were absent.
- Green check: `tools\verify_landscape_stage3.py` now passes.
- Build: `LandscapeEditor` Release builds with VS 18 CMake.
- Runtime D3D12: `build\Win64-vs18\smoke-landscape-editor-stage3-d3d12\landscape_editor_stage3_d3d12.png`.
- Runtime Vulkan: `build\Win64-vs18\smoke-landscape-editor-stage3-vk\landscape_editor_stage3_vk.png`.
- Pixel check: both captures are `640x480`, have 3 unique colors, 984 bright axis pixels, and 24670 non-background grid pixels.
- Architecture result: `LandscapeEditor` now calls `ForwardRenderer::Render()`; `ForwardDebugPipeline` is owned by `ForwardRenderer` and gets its grid PSO through project `PSOCache` warm-up.

## Follow-Up

The next slice will add the first CPU-generated flat terrain patch and submit it through the `Opaque` queue.
