cmake_minimum_required (VERSION 3.10)

project(LandscapeEditor CXX)

set(SOURCE
    src/FrameResources.cpp
    src/ForwardDebugPipeline.cpp
    src/ForwardRenderer.cpp
    src/LandscapeEditor.cpp
    src/PSOCache.cpp
    src/ShadowRenderer.cpp
    src/TerrainPatchRenderer.cpp
)

set(INCLUDE
    src/FrameResources.hpp
    src/ForwardDebugPipeline.hpp
    src/ForwardRenderer.hpp
    src/LandscapeEditor.hpp
    src/PSOCache.hpp
    src/RenderQueue.hpp
    src/RenderView.hpp
    src/ShadowRenderer.hpp
    src/TerrainPatchRenderer.hpp
)

add_sample_app(LandscapeEditor
    IDE_FOLDER Landscape
    SOURCES ${SOURCE}
    INCLUDES ${INCLUDE}
)
