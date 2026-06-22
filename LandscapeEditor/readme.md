# LandscapeEditor

`LandscapeEditor` is the first standalone terrain editor target for this project.

The first milestone keeps the runtime deliberately small:

- create a normal Diligent sample window;
- initialize a `ForwardDebugPipeline`;
- render a procedural triangle;
- verify the target through D3D12 and Vulkan smoke captures.

This target will grow into the terrain bring-up app for grid rendering, heightmap patches, quadtree LOD, and debug visualization.

## Build

```powershell
cd E:\Landscape
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build\Win64-vs18 --config Release --target LandscapeEditor --parallel
```

## D3D12 Smoke Run

```powershell
cd E:\Landscape\build\Win64-vs18\LandscapeEditor\Release
.\LandscapeEditor.exe --mode d3d12 --adapters_dialog 0 --golden_image_mode capture --capture_path E:\Landscape\build\Win64-vs18\smoke-landscape-editor-d3d12 --capture_name landscape_editor_d3d12 --capture_format png --capture_alpha 0 --show_ui 0 -w 640 -h 480
```
