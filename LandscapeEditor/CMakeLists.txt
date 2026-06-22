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
    IDE_FOLDER Landscape
    SOURCES ${SOURCE}
    INCLUDES ${INCLUDE}
)
