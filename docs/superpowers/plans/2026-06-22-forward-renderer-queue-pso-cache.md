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

## Follow-Up

The next slice will add the first CPU-generated flat terrain patch and submit it through the `Opaque` queue.
