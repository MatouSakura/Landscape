cmake_minimum_required (VERSION 3.10)

project(LandscapeEditor CXX)

set(SOURCE
    src/FrameResources.cpp
    src/ForwardDebugPipeline.cpp
    src/ForwardRenderer.cpp
    src/LandscapeEditor.cpp
    src/PostProcessRenderer.cpp
    src/PSOCache.cpp
    src/ShadowRenderer.cpp
    src/SkyRenderer.cpp
    src/TerrainHeightField.cpp
    src/TerrainPatchRenderer.cpp
    src/TerrainQuadtree.cpp
    src/TerrainQuadtreeDebugRenderer.cpp
    src/TransparentRenderer.cpp
)

set(INCLUDE
    src/FrameResources.hpp
    src/ForwardDebugPipeline.hpp
    src/ForwardRenderer.hpp
    src/LandscapeEditor.hpp
    src/PostProcessRenderer.hpp
    src/PSOCache.hpp
    src/RenderQueue.hpp
    src/RenderView.hpp
    src/ShadowRenderer.hpp
    src/SkyRenderer.hpp
    src/TerrainHeightField.hpp
    src/TerrainPatchRenderer.hpp
    src/TerrainQuadtree.hpp
    src/TerrainQuadtreeDebugRenderer.hpp
    src/TransparentRenderer.hpp
)

add_sample_app(LandscapeEditor
    IDE_FOLDER Landscape
    SOURCES ${SOURCE}
    INCLUDES ${INCLUDE}
)
