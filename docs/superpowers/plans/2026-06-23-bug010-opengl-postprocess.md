# BUG-010 OpenGL Postprocess Fix

Date: 2026-06-23

## Goal

Remove the temporary OpenGL `CopyTexture` postprocess fallback and make OpenGL use shader-based scene-color postprocessing like D3D12, Vulkan, and D3D11.

## Implementation

- Added GLSL postprocess vertex/pixel shaders in `PostProcessRenderer`.
- Kept HLSL postprocess shaders for D3D12, Vulkan, and D3D11.
- Selected `SHADER_SOURCE_LANGUAGE_GLSL` only for OpenGL devices.
- Removed `m_UseShaderPostProcess` and the OpenGL `CopyTexture` fallback path.
- Detached render targets after postprocess draw, matching the DiligentFX postprocess pattern.
- Updated validation scripts to require the GLSL path and reject `CopyTexture` in `PostProcessRenderer`.
- Closed `BUG-010` in `PROJECT_STATUS.md`.

## Verification

- Red check before implementation:
  - `tools\verify_landscape_stage6.py` failed because GLSL shader path was missing and `CopyTexture` fallback was still present.
  - `tools\verify_landscape_forward_completion.py` failed for the same code issue and because `BUG-010` was still open.
- Build:
  - `cmake --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel`.
- Validation:
  - `tools\verify_landscape_stage2.py`
  - `tools\verify_landscape_stage3.py`
  - `tools\verify_landscape_stage4.py`
  - `tools\verify_landscape_stage5.py`
  - `tools\verify_landscape_stage6.py`
  - `tools\verify_landscape_forward_completion.py`
- Smoke captures:
  - D3D12: `build\Win64-vs18\smoke-bug010-d3d12\bug010_d3d12.png`.
  - Vulkan: `build\Win64-vs18\smoke-bug010-vk\bug010_vk.png`.
  - D3D11: `build\Win64-vs18\smoke-bug010-d3d11\bug010_d3d11.png`.
  - OpenGL: `build\Win64-vs18\smoke-bug010-gl\bug010_gl.png`.
