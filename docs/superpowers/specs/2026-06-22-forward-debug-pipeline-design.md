# Forward Debug Pipeline Design

Date: 2026-06-22

## Goal

Create the first Landscape-specific rendering slice on top of Diligent Engine.

This slice should add a standalone `LandscapeEditor` sample and a minimal `ForwardDebugPipeline` that can clear the frame, bind a graphics PSO, and draw simple debug geometry. It is the bring-up path for later terrain work, not the final renderer.

## Non-Goals

- Do not implement deferred rendering in this slice.
- Do not implement quadtree LOD yet.
- Do not implement heightmap loading yet.
- Do not add editor features.
- Do not fork or rewrite Diligent Engine internals unless required for the sample to build.

## Location

Use the Diligent sample structure:

```text
DiligentSamples/Samples/LandscapeEditor
  CMakeLists.txt
  readme.md
  src/
    LandscapeEditor.hpp
    LandscapeEditor.cpp
    ForwardDebugPipeline.hpp
    ForwardDebugPipeline.cpp
```

Register it from:

```text
DiligentSamples/Samples/CMakeLists.txt
```

This keeps the first prototype close to existing sample infrastructure and reuses `SampleBase`.

## Architecture

### LandscapeEditor

`LandscapeEditor` owns the sample lifecycle.

Responsibilities:

- Inherit from `SampleBase`.
- Initialize the forward debug pipeline after Diligent has created the device, context, and swap chain.
- On each frame, clear the back buffer and call the forward debug pipeline.
- Use the existing sample app backend selection flow.

It should stay thin. Terrain logic should not accumulate directly in the sample class.

### ForwardDebugPipeline

`ForwardDebugPipeline` owns the first graphics PSO and draw path.

Responsibilities:

- Create a graphics PSO compatible with the swap chain color and depth formats.
- Use HLSL shader source in the same style as Diligent tutorials.
- Draw simple procedural geometry first, with a triangle as the first visual target.
- Avoid per-frame PSO creation.

The first version can use embedded shader strings. Later versions can move shaders into asset files when the terrain shader set grows.

### PSO Cache Direction

The first slice does not need a general-purpose PSO cache, but it must not block one.

Design constraints:

- PSO creation happens during initialization or explicit rebuild only.
- Render-time code receives already-created `IPipelineState` objects.
- Later cache keys should include backend, render target format, depth format, shader variant, material variant, vertex layout, and render state.

## Render Flow

Initial frame flow:

```text
LandscapeEditor::Render()
  Clear back buffer
  Clear depth buffer
  ForwardDebugPipeline::Render()
    SetPipelineState()
    Draw()
```

Future terrain flow:

```text
LandscapeEditor::Render()
  Clear frame
  ForwardDebugPipeline::RenderTerrainDebug()
  ForwardDebugPipeline::RenderQuadtreeOverlay()
  ForwardDebugPipeline::RenderUI()
```

## Error Handling

- If shader creation fails, Diligent should report the shader or PSO creation error.
- If PSO creation fails, initialization should fail loudly instead of silently skipping rendering.
- Render should guard against missing PSO only if initialization recovery is added later.

## Testing / Verification

Minimum verification:

1. Configure the project with Visual Studio 2022 CMake.
2. Build the `LandscapeEditor` target in `Release`.
3. Confirm the target exists in the generated solution.
4. Run the sample with at least one backend available on the machine.

Preferred backend verification:

- Vulkan on AMD RX 7900 XT.
- D3D12 on AMD RX 7900 XT.

If runtime verification is not possible in the current turn, record that as an open issue in `PROJECT_STATUS.md`.

## First Implementation Slice

The first implementation should:

1. Add `LandscapeEditor` sample files.
2. Add `ForwardDebugPipeline` files.
3. Register the sample in CMake.
4. Build only the new target or the whole solution if needed.
5. Commit and push the changes after writing files.

## Follow-Up Work

After the first forward debug sample renders:

- Replace procedural triangle with a flat grid.
- Add camera controls.
- Add a basic constant buffer for view/projection.
- Add wireframe mode.
- Add quadtree debug line rendering.
- Add heightmap terrain patch rendering.
