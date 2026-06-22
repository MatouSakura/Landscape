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
    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "queue_hpp": "LandscapeEditor/src/RenderQueue.hpp",
        "terrain_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "terrain_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("file_terrain_hpp", lambda: f("terrain_hpp")),
        ("file_terrain_cpp", lambda: f("terrain_cpp")),
        ("cmake_terrain", lambda: require_contains(f("cmake"), r"TerrainPatchRenderer\.(hpp|cpp)", "TerrainPatchRenderer in CMake")),
        ("terrain_vertex_buffer", lambda: require_contains(f("terrain_cpp"), r"BIND_VERTEX_BUFFER", "terrain vertex buffer")),
        ("terrain_index_buffer", lambda: require_contains(f("terrain_cpp"), r"BIND_INDEX_BUFFER", "terrain index buffer")),
        ("terrain_draw_indexed", lambda: require_contains(f("terrain_cpp"), r"DrawIndexed", "terrain indexed draw")),
        ("terrain_pso_cache", lambda: require_contains(f("terrain_cpp"), r"PSOCache\.GetOrCreate", "terrain PSO cache warm-up")),
        ("queue_opaque_submit", lambda: require_contains(f("queue_hpp"), r"AddTerrainPatch.*GetOpaqueItems.*GetQueueCount", "opaque terrain queue support")),
        ("renderer_owns_terrain", lambda: require_contains(f("renderer_hpp"), r"TerrainPatchRenderer\s+m_TerrainPatchRenderer", "ForwardRenderer terrain renderer member")),
        ("renderer_draws_opaque_before_debug", lambda: require_contains(f("renderer_cpp"), r"GetOpaqueItems\(\).*TerrainPatchRenderer\.Render.*GetDebugItems", "opaque draw before debug draw")),
        ("stats_opaque_triangles", lambda: require_contains(f("renderer_hpp"), r"OpaqueItemCount.*TerrainTriangleCount", "opaque stats")),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("stage4 verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("stage4 verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
