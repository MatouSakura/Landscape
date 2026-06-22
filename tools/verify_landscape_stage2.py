from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]


def read_text(rel_path: str) -> str:
    path = ROOT / rel_path
    if not path.exists():
        raise AssertionError(f"missing required file: {rel_path}")
    return path.read_text(encoding="utf-8")


def require_contains(text: str, pattern: str, description: str) -> None:
    if not re.search(pattern, text, re.MULTILINE | re.DOTALL):
        raise AssertionError(f"missing {description}")


def main() -> int:
    checks = []

    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "pipeline_hpp": "LandscapeEditor/src/ForwardDebugPipeline.hpp",
        "pipeline_cpp": "LandscapeEditor/src/ForwardDebugPipeline.cpp",
        "render_view": "LandscapeEditor/src/RenderView.hpp",
        "frame_resources_hpp": "LandscapeEditor/src/FrameResources.hpp",
        "frame_resources_cpp": "LandscapeEditor/src/FrameResources.cpp",
    }

    def require_file(key: str) -> str:
        return read_text(files[key])

    checks.append(("file_render_view", lambda: require_file("render_view")))
    checks.append(("file_frame_resources_hpp", lambda: require_file("frame_resources_hpp")))
    checks.append(("file_frame_resources_cpp", lambda: require_file("frame_resources_cpp")))
    checks.append(("cmake_render_view", lambda: require_contains(require_file("cmake"), r"RenderView\.hpp", "RenderView in CMake")))
    checks.append(("cmake_frame_resources", lambda: require_contains(require_file("cmake"), r"FrameResources\.(hpp|cpp)", "FrameResources in CMake")))
    checks.append(("render_view_fields", lambda: require_contains(require_file("render_view"), r"ViewProj.*CameraPosition.*NearPlane.*FarPlane", "RenderView camera fields")))
    checks.append(("frame_cb_owner", lambda: require_contains(require_file("frame_resources_hpp"), r"CameraConstants.*GetCameraConstantsBuffer", "FrameResources camera constants API")))
    checks.append(("frame_cb_update", lambda: require_contains(require_file("frame_resources_cpp"), r"MapHelper<\s*CameraConstants\s*>", "FrameResources dynamic map update")))
    checks.append(("camera_member", lambda: require_contains(require_file("editor_hpp"), r"FirstPersonCamera\s+m_Camera", "LandscapeEditor camera member")))
    checks.append(("render_view_member", lambda: require_contains(require_file("editor_hpp"), r"RenderView\s+m_RenderView", "LandscapeEditor RenderView member")))
    checks.append(("frame_resources_member", lambda: require_contains(require_file("editor_hpp"), r"FrameResources\s+m_FrameResources", "LandscapeEditor FrameResources member")))
    checks.append(("camera_update", lambda: require_contains(require_file("editor_cpp"), r"m_Camera\.Update\s*\(\s*GetInputController\(\)", "camera update from input controller")))
    checks.append(("pipeline_signature", lambda: require_contains(require_file("pipeline_hpp"), r"Render\s*\([^)]*const\s+RenderView&[^)]*FrameResources&", "ForwardDebugPipeline render signature")))
    checks.append(("shader_camera_cbuffer", lambda: require_contains(require_file("pipeline_cpp"), r"cbuffer\s+CameraConstants", "camera constants cbuffer in shader")))
    checks.append(("shader_view_proj", lambda: require_contains(require_file("pipeline_cpp"), r"ViewProj", "ViewProj usage in debug shader")))

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("stage2 verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("stage2 verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
