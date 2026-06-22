# Forward Debug Grid Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the first `LandscapeEditor` procedural triangle with a flat debug grid rendered by `ForwardDebugPipeline`.

**Architecture:** Keep the grid inside `ForwardDebugPipeline` as the first forward debug primitive. Use a procedural HLSL vertex shader with `SV_VertexID`, a line-list PSO, and no vertex buffers yet. This keeps the first grid backend-neutral and leaves world-space camera integration for the next milestone.

**Tech Stack:** C++20, Diligent Engine graphics PSO, HLSL, CMake, Visual Studio 18 generator, D3D11/D3D12/Vulkan/OpenGL smoke captures.

---

## File Structure

- Modify: `LandscapeEditor/src/ForwardDebugPipeline.hpp`
  - Rename the PSO member from triangle-specific naming to grid-specific naming.
- Modify: `LandscapeEditor/src/ForwardDebugPipeline.cpp`
  - Replace triangle shaders with procedural grid line shaders.
  - Change primitive topology from triangle list to line list.
  - Draw 44 vertices for 22 grid lines.
- Modify: `LandscapeEditor/readme.md`
  - Record that the first debug primitive is now a flat grid.
- Modify: `PROJECT_STATUS.md`
  - Record build and smoke results after implementation.

## Task 1: Write The Visual Regression Check

**Files:**
- No repository file required for the first red check.

- [ ] **Step 1: Run the current smoke capture**

```powershell
cd E:\Landscape\build\Win64-vs18\LandscapeEditor\Release
.\LandscapeEditor.exe --mode d3d12 --adapters_dialog 0 --golden_image_mode capture --capture_path E:\Landscape\build\Win64-vs18\smoke-landscape-editor-d3d12 --capture_name landscape_editor_d3d12 --capture_format png --capture_alpha 0 --show_ui 0 -w 640 -h 480
```

Expected: command exits with code `0` and produces the current triangle capture.

- [ ] **Step 2: Run a pixel check that fails for the triangle**

Use the bundled Python runtime to verify there are many bright pixels on both the horizontal and vertical center lines. The triangle does not satisfy this grid-like pattern.

```powershell
$py = "C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
& $py - <<'PY'
from PIL import Image
img = Image.open(r"E:\Landscape\build\Win64-vs18\smoke-landscape-editor-d3d12\landscape_editor_d3d12.png").convert("RGB")
w, h = img.size
row = [img.getpixel((x, h // 2)) for x in range(w)]
col = [img.getpixel((w // 2, y)) for y in range(h)]
def bright_count(pixels):
    return sum(1 for r, g, b in pixels if max(r, g, b) - min(r, g, b) < 48 and r + g + b > 390)
assert bright_count(row) > 500 and bright_count(col) > 350, "expected visible horizontal and vertical grid axes"
PY
```

Expected: assertion failure before the grid implementation.

## Task 2: Implement The Procedural Grid

**Files:**
- Modify: `LandscapeEditor/src/ForwardDebugPipeline.hpp`
- Modify: `LandscapeEditor/src/ForwardDebugPipeline.cpp`

- [ ] **Step 1: Rename the PSO member**

Change:

```cpp
RefCntAutoPtr<IPipelineState> m_pTrianglePSO;
```

to:

```cpp
RefCntAutoPtr<IPipelineState> m_pGridPSO;
```

- [ ] **Step 2: Replace shader sources**

Use a vertex shader that draws eleven vertical lines and eleven horizontal lines in clip space. Axis lines are brighter than minor lines.

```hlsl
static const uint GridResolution = 10;
static const uint LinesPerAxis   = GridResolution + 1;
static const uint VertLinesEnd   = LinesPerAxis * 2;
uint LineVertex = VertId & 1u;
uint LineIndex  = VertId >> 1u;
```

Map line coordinates from `[-0.8, +0.8]` with:

```hlsl
float Coord = -GridExtent + GridExtent * 2.0 * float(LineIndexOrLocalIndex) / float(GridResolution);
```

- [ ] **Step 3: Change the pipeline topology**

Change:

```cpp
PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
```

to:

```cpp
PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_LINE_LIST;
```

- [ ] **Step 4: Draw all grid vertices**

Draw `44` vertices:

```cpp
DrawAttribs DrawAttrs;
DrawAttrs.NumVertices = 44;
pContext->Draw(DrawAttrs);
```

## Task 3: Verify And Document

**Files:**
- Modify: `LandscapeEditor/readme.md`
- Modify: `PROJECT_STATUS.md`

- [ ] **Step 1: Build**

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Expected: exit code `0`, `LandscapeEditor.exe` updated.

- [ ] **Step 2: Smoke run four backends**

Run `LandscapeEditor.exe` with `--mode d3d12`, `--mode vk`, `--mode d3d11`, and `--mode gl`, using golden-image capture and `--show_ui 0`.

Expected: each process exits with code `0`, and each backend produces a non-empty `landscape_editor_<mode>.png`.

- [ ] **Step 3: Run the pixel check again**

Run the same bundled-Python pixel check from Task 1.

Expected: the assertion passes after the grid implementation.

- [ ] **Step 4: Update docs and commit**

```powershell
git add LandscapeEditor PROJECT_STATUS.md docs\superpowers\plans\2026-06-22-forward-debug-grid.md
git commit -m "Add forward debug grid"
git push origin master
```

## Self-Review

- Spec coverage: implements the roadmap item "Replace the procedural triangle with a flat debug grid."
- Scope: does not introduce camera matrices, terrain patch buffers, PSO cache, or UI.
- Testability: includes a red/green visual regression check against the D3D12 smoke capture plus four-backend smoke runs.
- Type consistency: `ForwardDebugPipeline::Initialize()` and `ForwardDebugPipeline::Render()` signatures remain unchanged.
