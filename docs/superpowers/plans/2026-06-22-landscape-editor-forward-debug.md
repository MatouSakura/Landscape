# LandscapeEditor Forward Debug Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first runnable `LandscapeEditor` sample with a minimal `ForwardDebugPipeline` that renders a procedural triangle through Diligent Engine.

**Architecture:** `LandscapeEditor` will live under `DiligentSamples/Samples/LandscapeEditor` and use Diligent's existing `add_sample_app()` integration. The editor sample owns application lifecycle through `SampleBase`, while `ForwardDebugPipeline` owns the first graphics PSO and draw call. This keeps the first app small and gives later terrain code a clear rendering entry point.

**Tech Stack:** C++20, CMake, Diligent Engine SampleBase, HLSL shaders embedded as source strings for the first smoke target.

---

## File Structure

- Modify: `DiligentSamples/Samples/CMakeLists.txt`
  - Register `LandscapeEditor` with `add_subdirectory(LandscapeEditor)`.
- Create: `DiligentSamples/Samples/LandscapeEditor/CMakeLists.txt`
  - Define the `LandscapeEditor` target through `add_sample_app()`.
- Create: `DiligentSamples/Samples/LandscapeEditor/readme.md`
  - Explain the first editor milestone and smoke commands.
- Create: `DiligentSamples/Samples/LandscapeEditor/assets/.gitignore`
  - Keep the assets directory present because `add_sample_app()` copies it after build.
- Create: `DiligentSamples/Samples/LandscapeEditor/src/ForwardDebugPipeline.hpp`
  - Declare the small render pipeline wrapper.
- Create: `DiligentSamples/Samples/LandscapeEditor/src/ForwardDebugPipeline.cpp`
  - Build the triangle PSO and issue the draw call.
- Create: `DiligentSamples/Samples/LandscapeEditor/src/LandscapeEditor.hpp`
  - Declare the sample class.
- Create: `DiligentSamples/Samples/LandscapeEditor/src/LandscapeEditor.cpp`
  - Wire SampleBase initialization, clear, render, and UI update.
- Modify: `PROJECT_STATUS.md`
  - Record the new target and validation result after implementation.

## Task 1: Register The Sample Shell

**Files:**
- Modify: `DiligentSamples/Samples/CMakeLists.txt`
- Create: `DiligentSamples/Samples/LandscapeEditor/CMakeLists.txt`
- Create: `DiligentSamples/Samples/LandscapeEditor/readme.md`
- Create: `DiligentSamples/Samples/LandscapeEditor/assets/.gitignore`

- [ ] **Step 1: Verify the target is absent**

Run:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Expected: MSBuild reports that target `LandscapeEditor` does not exist.

- [ ] **Step 2: Register `LandscapeEditor`**

Add this line to `DiligentSamples/Samples/CMakeLists.txt` near the other always-built samples:

```cmake
add_subdirectory(LandscapeEditor)
```

- [ ] **Step 3: Add the sample target CMake file**

Create `DiligentSamples/Samples/LandscapeEditor/CMakeLists.txt`:

```cmake
cmake_minimum_required (VERSION 3.10)

project(LandscapeEditor CXX)

set(SOURCE
    src/ForwardDebugPipeline.cpp
    src/LandscapeEditor.cpp
)

set(INCLUDE
    src/ForwardDebugPipeline.hpp
    src/LandscapeEditor.hpp
)

add_sample_app(LandscapeEditor
    IDE_FOLDER DiligentSamples/Samples
    SOURCES ${SOURCE}
    INCLUDES ${INCLUDE}
)
```

- [ ] **Step 4: Add a short readme and empty assets directory marker**

Create `readme.md` with the current milestone and smoke commands. Create `assets/.gitignore` so the assets directory exists in git.

## Task 2: Add ForwardDebugPipeline

**Files:**
- Create: `DiligentSamples/Samples/LandscapeEditor/src/ForwardDebugPipeline.hpp`
- Create: `DiligentSamples/Samples/LandscapeEditor/src/ForwardDebugPipeline.cpp`

- [ ] **Step 1: Declare the pipeline wrapper**

`ForwardDebugPipeline` exposes:

```cpp
void Initialize(IRenderDevice* pDevice, ISwapChain* pSwapChain);
void Render(IDeviceContext* pContext);
```

It stores only `RefCntAutoPtr<IPipelineState> m_pTrianglePSO`.

- [ ] **Step 2: Implement the triangle PSO**

Use embedded HLSL vertex/pixel shader strings. Configure:

- `PIPELINE_TYPE_GRAPHICS`
- one render target using the swap-chain color format
- swap-chain depth format
- `PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`
- `CULL_MODE_NONE`
- `DepthEnable = False`

- [ ] **Step 3: Implement the draw call**

Bind the PSO and draw three vertices:

```cpp
DrawAttribs DrawAttrs;
DrawAttrs.NumVertices = 3;
pContext->Draw(DrawAttrs);
```

## Task 3: Add LandscapeEditor App

**Files:**
- Create: `DiligentSamples/Samples/LandscapeEditor/src/LandscapeEditor.hpp`
- Create: `DiligentSamples/Samples/LandscapeEditor/src/LandscapeEditor.cpp`

- [ ] **Step 1: Declare the sample**

`LandscapeEditor` derives from `SampleBase`, overrides `Initialize`, `Render`, `Update`, and returns sample name `LandscapeEditor`.

- [ ] **Step 2: Implement SampleBase wiring**

`Initialize()` calls `SampleBase::Initialize(InitInfo)` and then `m_ForwardDebugPipeline.Initialize(m_pDevice, m_pSwapChain)`.

- [ ] **Step 3: Implement rendering**

`Render()` clears the swap-chain back buffer to a dark neutral color, clears depth, and calls `m_ForwardDebugPipeline.Render(m_pImmediateContext)`.

- [ ] **Step 4: Keep UI update minimal**

`Update()` calls `SampleBase::Update(CurrTime, ElapsedTime, DoUpdateUI)` only. Debug UI will be added after the first target is stable.

## Task 4: Configure, Build, Smoke Run, Document

**Files:**
- Modify: `PROJECT_STATUS.md`

- [ ] **Step 1: Reconfigure CMake**

Run:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build\Win64-vs18 -G "Visual Studio 18 2026" -A x64 -DPython3_EXECUTABLE="C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
```

Expected: configure completes and generates a `LandscapeEditor` target.

- [ ] **Step 2: Build the new target**

Run:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Expected: `LandscapeEditor.exe` is produced under `build\Win64-vs18\DiligentSamples\Samples\LandscapeEditor\Release`.

- [ ] **Step 3: Smoke run D3D12**

Run:

```powershell
cd E:\Landscape\build\Win64-vs18\DiligentSamples\Samples\LandscapeEditor\Release
.\LandscapeEditor.exe --mode d3d12 --adapters_dialog 0 --golden_image_mode capture --capture_path E:\Landscape\build\Win64-vs18\smoke-landscape-editor-d3d12 --capture_name landscape_editor_d3d12 --capture_format png --capture_alpha 0 --show_ui 0 -w 640 -h 480
```

Expected: exit code `0`, one PNG capture, and a visible triangle.

- [ ] **Step 4: Smoke run Vulkan**

Run the same command with `--mode vk` and `smoke-landscape-editor-vk`.

Expected: exit code `0`, one PNG capture, and a visible triangle.

- [ ] **Step 5: Update status and commit**

Update `PROJECT_STATUS.md` with build and smoke results, then commit and push.

```powershell
git add DiligentSamples/Samples/CMakeLists.txt DiligentSamples/Samples/LandscapeEditor PROJECT_STATUS.md
git commit -m "Add LandscapeEditor forward debug sample"
git push origin master
```

## Self-Review

- Spec coverage: this plan implements the first `LandscapeEditor` target and the first `ForwardDebugPipeline` milestone from the forward renderer design.
- Scope: terrain grids, camera controls, PSO cache, and quadtree LOD are intentionally deferred until the executable target is stable.
- Validation: the plan includes CMake reconfigure, target build, and D3D12/Vulkan smoke captures.
- Ambiguity: `LandscapeEditor` is placed inside `DiligentSamples/Samples` for the first milestone; later migration to a separate top-level app remains a separate architecture decision.
