# Forward Pipeline Hardening Plan

Date: 2026-06-22

## Goal

Close the first complete `LandscapeEditor` forward renderer milestone.

This slice does not add new visual features. It hardens the completed pass chain, records final backend smoke results, verifies that PSOs are created through initialization-time caches instead of the draw loop, and updates project documentation with the current done/next/bug state.

## Implementation

- Add a final forward-completion verification script.
- Verify Stage 2 through Stage 6 validation scripts still pass.
- Verify renderer `Render()` methods do not call `PSOCache::GetOrCreate`.
- Re-run final D3D12, Vulkan, D3D11, and OpenGL smoke captures.
- Update `PROJECT_STATUS.md` and the complete forward pipeline plan with final results.
- Keep the OpenGL postprocess shader-sampling issue documented as an unresolved compatibility bug.

## Verification

- Build `LandscapeEditor` Release.
- Run final D3D12, Vulkan, D3D11, and OpenGL smoke captures.
- Check captures are non-empty and contain sky/grid/terrain/transparent content.
- Confirm final validation scripts pass.
- Commit and push the hardening record to `origin/master`.
