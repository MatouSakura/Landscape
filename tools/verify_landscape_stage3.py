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


def require_not_contains(text: str, pattern: str, description: str) -> None:
    if re.search(pattern, text, re.MULTILINE | re.DOTALL):
        raise AssertionError(f"unexpected {description}")


def main() -> int:
    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "debug_hpp": "LandscapeEditor/src/ForwardDebugPipeline.hpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "queue_hpp": "LandscapeEditor/src/RenderQueue.hpp",
        "pso_hpp": "LandscapeEditor/src/PSOCache.hpp",
        "pso_cpp": "LandscapeEditor/src/PSOCache.cpp",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("file_forward_renderer_hpp", lambda: f("renderer_hpp")),
        ("file_forward_renderer_cpp", lambda: f("renderer_cpp")),
        ("file_render_queue", lambda: f("queue_hpp")),
        ("file_pso_cache_hpp", lambda: f("pso_hpp")),
        ("file_pso_cache_cpp", lambda: f("pso_cpp")),
        ("cmake_forward_renderer", lambda: require_contains(f("cmake"), r"ForwardRenderer\.(hpp|cpp)", "ForwardRenderer in CMake")),
        ("cmake_render_queue", lambda: require_contains(f("cmake"), r"RenderQueue\.hpp", "RenderQueue in CMake")),
        ("cmake_pso_cache", lambda: require_contains(f("cmake"), r"PSOCache\.(hpp|cpp)", "PSOCache in CMake")),
        ("editor_owns_renderer", lambda: require_contains(f("editor_hpp"), r"ForwardRenderer\s+m_ForwardRenderer", "LandscapeEditor ForwardRenderer member")),
        ("editor_no_debug_pipeline", lambda: require_not_contains(f("editor_hpp"), r"ForwardDebugPipeline\s+m_ForwardDebugPipeline", "direct ForwardDebugPipeline member in editor")),
        ("editor_calls_renderer", lambda: require_contains(f("editor_cpp"), r"m_ForwardRenderer\.Render\s*\(", "LandscapeEditor calls ForwardRenderer::Render")),
        ("renderer_owns_parts", lambda: require_contains(f("renderer_hpp"), r"ForwardDebugPipeline.*RenderQueue.*PSOCache", "ForwardRenderer owned debug pipeline, queue, and PSO cache")),
        ("renderer_stats", lambda: require_contains(f("renderer_hpp"), r"ForwardRendererStats.*DebugItemCount.*PSOCount.*PSOCreationCount", "ForwardRenderer stats")),
        ("pso_get_or_create", lambda: require_contains(f("pso_hpp"), r"GetOrCreate", "PSOCache GetOrCreate API")),
        ("debug_uses_pso_cache", lambda: require_contains(f("debug_hpp"), r"Initialize\s*\([^)]*PSOCache&", "ForwardDebugPipeline initialized with PSOCache")),
        ("queue_debug_submit", lambda: require_contains(f("queue_hpp"), r"AddDebugGrid.*GetQueueCount", "RenderQueue debug submit and count")),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("stage3 verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("stage3 verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
