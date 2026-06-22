# LandscapeEditor Complete Forward Pipeline Plan

Date: 2026-06-22

## Goal

Bring `LandscapeEditor` from the current debug grid to a complete first forward renderer suitable for terrain-system development.

Completion means the editor has camera-driven rendering, frame constants, render queues, a PSO cache, forward opaque terrain patch rendering, sun light with four-cascade shadow maps, procedural sky, transparent/debug/postprocess passes, ImGui diagnostics, and smoke coverage across D3D12, Vulkan, D3D11, and OpenGL.

## Execution Rules

- After each implementation slice, update this project documentation and `PROJECT_STATUS.md`.
- Build `LandscapeEditor` after every tracked file write group.
- Commit after a successful build.
- Push each commit to `origin/master`.
- Keep custom Landscape code in root-owned folders, not inside Diligent submodules.
- Keep RTX/NVIDIA-only features out of the runtime path; the AMD RX 7900 XT must remain supported.

## Milestones

1. Synchronize docs and architecture decisions with the current root-owned `LandscapeEditor/` layout.
2. Add `FirstPersonCamera`, `RenderView`, and `FrameResources`, then move the debug grid into world space.
3. Add `ForwardRenderer`, `RenderQueue`, and project `PSOCache`.
4. Add a CPU-generated flat terrain patch and draw it through `ForwardOpaque`.
5. Add directional sun light and four-cascade shadow rendering.
6. Add procedural sky, transparent queue, tone mapping, and debug UI controls.
7. Harden documentation, verify PSO creation is stable after initialization, run final backend smoke tests, and push the completed state.

## Verification

Primary build command:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Primary smoke command template:

```powershell
cd E:\Landscape\build\Win64-vs18\LandscapeEditor\Release
.\LandscapeEditor.exe --mode d3d12 --adapters_dialog 0 --golden_image_mode capture --capture_path E:\Landscape\build\Win64-vs18\smoke-landscape-editor-d3d12 --capture_name landscape_editor_d3d12 --capture_format png --capture_alpha 0 --show_ui 0 -w 640 -h 480
```

Final verification must run D3D12, Vulkan, D3D11, and OpenGL smoke captures.

## Current Slice

The first complete forward pipeline milestone is complete as of 2026-06-22.

Completed slices:

- Root-owned `LandscapeEditor/` documentation and architecture sync.
- Camera-driven `RenderView` and `FrameResources`.
- `ForwardRenderer`, `RenderQueue`, and `PSOCache`.
- CPU-generated flat terrain patch in `ForwardOpaque`.
- Sun light constants and four-cascade shadow resources.
- Procedural sky pass.
- Transparent queue and alpha-blended transparent quad.
- Scene-color target and postprocess pass boundary.
- Final hardening script and four-backend smoke captures.

Final validation:

- Build: `cmake --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel`.
- Validation scripts: `tools\verify_landscape_stage2.py` through `tools\verify_landscape_stage6.py`, plus `tools\verify_landscape_forward_completion.py`.
- Final D3D12 smoke: `build\Win64-vs18\smoke-landscape-editor-final-d3d12\landscape_editor_final_d3d12.png`.
- Final Vulkan smoke: `build\Win64-vs18\smoke-landscape-editor-final-vk\landscape_editor_final_vk.png`.
- Final D3D11 smoke: `build\Win64-vs18\smoke-landscape-editor-final-d3d11\landscape_editor_final_d3d11.png`.
- Final OpenGL smoke: `build\Win64-vs18\smoke-landscape-editor-final-gl\landscape_editor_final_gl.png`.

OpenGL note:

- OpenGL uses a `CopyTexture` fallback for the postprocess boundary because the shader scene-color sampling path crashes during golden-image capture on this machine. This is tracked in `PROJECT_STATUS.md` as `BUG-010`.
