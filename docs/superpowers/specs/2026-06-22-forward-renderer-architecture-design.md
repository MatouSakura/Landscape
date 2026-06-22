# Forward Renderer Architecture Design

Date: 2026-06-22

## Goal

Define the normal forward renderer architecture for the Landscape project.

This document complements `2026-06-22-forward-debug-pipeline-design.md`. The debug pipeline is the first bring-up slice. The architecture here defines the longer-lived renderer shape that will support terrain, shadows, sky, transparent objects, debug drawing, UI, post-processing, and later deferred rendering integration.

## Scope

This design covers:

- Forward renderer ownership and boundaries.
- Render pass order.
- Draw queues.
- PSO cache responsibilities.
- Frame resources.
- Terrain integration points.
- Error handling and verification.

This design does not implement deferred rendering. It only reserves boundaries so the future deferred renderer can share scene data, terrain data, frame resources, and PSO cache infrastructure.

## Design Direction

Use a layered renderer:

```text
LandscapeEditor / App
  Scene and camera setup
  Runtime UI and settings
  Calls renderer per frame

ForwardRenderer
  Owns forward pass orchestration
  Builds and consumes render queues
  Owns forward-specific frame resources
  Uses PSO cache

Render Systems
  TerrainRenderer
  SkyRenderer
  ShadowRenderer
  DebugRenderer
  UIRenderer

Renderer Infrastructure
  PSOCache
  ShaderLibrary
  FrameResources
  RenderQueue
  RenderView
```

The forward renderer should orchestrate rendering. It should not own terrain data or scene authoring data.

## Main Components

### ForwardRenderer

`ForwardRenderer` is the main runtime renderer for the forward path.

Responsibilities:

- Initialize forward pass resources.
- Own frame-level render flow.
- Resize render targets when the swap chain changes.
- Build or consume render queues.
- Execute passes in a stable order.
- Expose debug settings through a small settings struct.
- Keep render-time code free of PSO creation.

Non-responsibilities:

- Loading heightmaps.
- Managing quadtree LOD data.
- Owning terrain tile streaming.
- Owning editor tools.

### RenderView

`RenderView` describes one camera view.

Fields:

- View matrix.
- Projection matrix.
- View-projection matrix.
- Camera position.
- Near and far plane.
- Viewport size.
- Frustum planes.
- Reverse-Z flag.
- Output color format.
- Output depth format.

The renderer should be able to render from a `RenderView` without reading camera state directly from the app.

### FrameResources

`FrameResources` stores per-frame GPU resources.

Initial resources:

- Camera constant buffer.
- Frame constant buffer.
- Light constant buffer.
- Dynamic upload buffers if needed.

Later resources:

- Per-draw instance buffer.
- Indirect draw argument buffer.
- GPU culling buffers.
- Temporal history resources.

Frame resources should be recreated or resized only when needed, not every frame.

### RenderQueue

`RenderQueue` contains renderable items for a pass.

Initial queues:

- `OpaqueQueue`
- `MaskedQueue`
- `TransparentQueue`
- `DebugQueue`

Each `RenderItem` should contain enough information to draw without asking the scene object for more state.

Initial `RenderItem` data:

- Mesh or geometry handle.
- Material handle.
- World transform or instance range.
- Bounding box.
- Sort key.
- PSO key.
- Shader resource binding handle.

### PSOCache

`PSOCache` owns runtime pipeline state lookup and creation.

First implementation:

- In-memory map from `PSOKey` to `IPipelineState`.
- Synchronous PSO creation during initialization or explicit warm-up.
- No PSO creation during normal draw loops.

Later implementation:

- Integrate Diligent `RenderDeviceWithCache` and `RenderStateCache`.
- Store cache files per backend and build configuration.
- Allow shader hot reload in development builds.
- Add async PSO compilation for expensive variants.

The project should use two levels of caching:

1. Project-level `PSOCache` maps engine concepts to Diligent PSO objects.
2. Diligent `RenderStateCache` handles backend-level state serialization and reuse.

### PSOKey

`PSOKey` should describe the full state needed to select a pipeline.

Initial fields:

- Backend type.
- Pipeline path: forward, forward debug, shadow, transparent.
- Pass type.
- Shader variant id.
- Material variant id.
- Vertex layout id.
- Render target format.
- Depth format.
- Primitive topology.
- Rasterizer state id.
- Depth stencil state id.
- Blend state id.
- Sample count.
- Reverse-Z flag.

The key must be deterministic and comparable.

### ShaderLibrary

`ShaderLibrary` is a thin owner for shader source lookup and variant creation.

First implementation:

- Embedded HLSL strings are acceptable for `ForwardDebugPipeline`.
- Real forward renderer shaders should move into files when terrain material work begins.

Later implementation:

- Central shader include paths.
- Shader macros for variants.
- Hot reload support through Diligent RenderStateCache in development builds.

## Forward Pass Order

The stable forward frame order should be:

```text
BeginFrame
  Update frame constants
  Prepare render queues

ShadowPass
  Render shadow casters into shadow maps

DepthPrepass optional
  Render opaque depth only when enabled

ForwardOpaquePass
  Render opaque terrain and opaque meshes

ForwardMaskedPass
  Render alpha-tested terrain details and foliage

SkyPass
  Render sky and atmosphere

ForwardTransparentPass
  Render transparent objects, water, and effects

DebugPass
  Render lines, wireframe overlays, quadtree overlays, bounding boxes

PostProcessPass
  Tone mapping and simple screen-space effects

UIPass
  ImGui and runtime tooling

Present
```

The first implementation does not need all passes. It should create the class boundaries so passes can be added without rewriting the app.

## Terrain Integration

`TerrainRenderer` should expose pass-oriented methods:

```cpp
void BuildRenderItems(const RenderView& View, RenderQueueBuilder& Builder);
void DrawDepth(const RenderPassContext& Context, const RenderQueue& Queue);
void DrawForwardOpaque(const RenderPassContext& Context, const RenderQueue& Queue);
void DrawForwardDebug(const RenderPassContext& Context, const TerrainDebugOptions& Options);
void DrawShadow(const RenderPassContext& Context, const RenderQueue& Queue);
```

The terrain system should own:

- Heightmap data.
- Quadtree nodes.
- Tile streaming state.
- LOD selection.
- Terrain material parameters.

The renderer should own:

- Active pass order.
- Render target binding.
- PSO binding.
- Frame constants.
- Draw queue execution.

This keeps terrain logic reusable by both forward and deferred renderers.

## Render Target Strategy

Initial forward renderer:

- Render directly into the swap chain back buffer.
- Use the swap chain depth buffer.

Next forward renderer:

- Render into an HDR color target.
- Resolve or tone map into the swap chain.
- Keep a depth target suitable for post-processing and picking.

Future resources:

- Shadow maps.
- Scene color HDR target.
- Scene depth target.
- Optional velocity target for temporal effects.
- Optional object id target for picking.

## Sorting and State Bucketing

Initial sorting:

- Opaque: sort by PSO key, material id, mesh id.
- Masked: sort by PSO key, material id, mesh id.
- Transparent: sort back to front by distance.
- Debug: preserve submission order unless a reason appears to sort.

The first terrain-only version can keep sorting simple, but render queue data structures should not block later state bucketing.

## Pipeline Switching

Runtime pipeline switching should work like this:

```text
RendererMode = ForwardDebug
  Use ForwardDebugPipeline and its PSOs

RendererMode = Forward
  Use ForwardRenderer and forward PSOs

RendererMode = Deferred later
  Use DeferredRenderer and deferred PSOs
```

Switching modes must not compile shaders or create PSOs during normal frame rendering.

Allowed creation points:

- Renderer initialization.
- Explicit warm-up step.
- Explicit settings change that invalidates a pipeline.
- Development hot reload command.

## Diligent Integration

The first sample should use direct Diligent objects:

- `IRenderDevice`
- `IDeviceContext`
- `ISwapChain`
- `IPipelineState`
- `IShaderResourceBinding`
- `IBuffer`

For state caching, use a staged approach:

1. First slice: store `RefCntAutoPtr<IPipelineState>` in small renderer classes.
2. Second slice: introduce project `PSOCache`.
3. Third slice: wrap PSO creation with Diligent `RenderDeviceWithCache`.
4. Fourth slice: load and save Diligent `RenderStateCache` files per backend.

This follows Diligent examples such as `Tutorial26_StateCache` and `USDViewer`, but keeps the first terrain renderer easy to debug.

## Error Handling

Initialization should fail loudly when required GPU objects cannot be created.

Rules:

- Missing shader: log the shader name and fail initialization.
- Missing PSO: log the PSO key and fail initialization.
- Missing render target: log the pass name and skip no silently.
- Unsupported backend feature: disable the feature and show it in debug UI if there is a fallback.

No normal frame should silently skip the terrain draw due to missing PSO or missing SRB.

## Debug UI

First useful controls:

- Renderer mode: ForwardDebug / Forward.
- Wireframe toggle.
- Show quadtree nodes.
- Show LOD colors.
- Freeze LOD selection.
- Show render queue counts.
- Show PSO count.
- Show PSO changes per frame if available from Diligent context stats.

Debug UI should not own renderer state directly. It should edit a `ForwardRendererSettings` or `TerrainDebugOptions` struct.

## Metrics

Track at least:

- Draw call count.
- Triangle count.
- Render queue item count.
- PSO count.
- PSO changes per frame.
- Visible terrain node count.
- Culled terrain node count.

These metrics are useful before adding GPU culling.

## Current Milestones

The first milestones are already complete:

1. Add root-owned `LandscapeEditor`.
2. Add `ForwardDebugPipeline`.
3. Draw procedural debug geometry.
4. Replace the triangle with a flat debug grid.
5. Build and smoke-test D3D12, Vulkan, D3D11, and OpenGL.

This validates app integration, PSO creation, and AMD-compatible backend execution.

## Next Milestone

The next milestone is the first complete forward renderer core:

1. Add `ForwardRenderer` skeleton.
2. Add `RenderView`.
3. Add `FrameResources`.
4. Add a small in-memory `PSOCache`.
5. Move debug grid draw submission through a simple `RenderQueue`.
6. Add debug UI for renderer mode and metrics.

## Full Forward Pipeline Milestone

After the renderer core is stable:

1. Add camera constants and world/view/projection transforms.
2. Add a flat terrain patch abstraction.
3. Add forward opaque terrain drawing.
4. Add directional sun light and four-cascade shadow maps.
5. Add procedural sky, transparent queue, simple tone mapping, and ImGui debug controls.

## Verification

Build verification:

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build\Win64-vs18 -G "Visual Studio 18 2026" -A x64 -DPython3_EXECUTABLE="C:\Users\liuyuan\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

Runtime verification:

- Launch `LandscapeEditor`.
- Confirm the window clears to the expected color.
- Confirm the procedural geometry renders.
- Confirm no PSO creation occurs during the draw loop after initialization.
- Confirm at least one AMD-supported backend runs.

Preferred backend order:

1. Vulkan.
2. D3D12.
3. D3D11 fallback if needed.

## Resolved Direction

- `LandscapeEditor` stays in the root-owned `LandscapeEditor/` directory.
- The first project `PSOCache` uses custom deterministic keys and in-memory Diligent PSO references.
- Diligent `RenderStateCache` integration remains a later optimization after the forward pipeline is stable.
- The first terrain surface uses CPU-generated vertex and index buffers so the same geometry can feed forward, shadow, debug, and later quadtree passes.
