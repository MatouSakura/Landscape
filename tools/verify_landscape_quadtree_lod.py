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


def iter_render_function_bodies(text: str):
    pattern = re.compile(r"\bvoid\s+([A-Za-z0-9_]+::Render)\s*\([^)]*\)\s*\{", re.MULTILINE)
    for match in pattern.finditer(text):
        start = match.end() - 1
        depth = 0
        for index in range(start, len(text)):
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1
                if depth == 0:
                    yield match.group(1), text[start : index + 1]
                    break


def require_no_pso_creation_in_render(rel_paths: list[str]) -> None:
    violations = []
    for rel_path in rel_paths:
        text = read_text(rel_path)
        for name, body in iter_render_function_bodies(text):
            if "GetOrCreate" in body or "CreateGraphicsPipelineState" in body:
                violations.append(f"{rel_path}: {name}")
    if violations:
        raise AssertionError("PSO creation/cache lookup found in Render(): " + ", ".join(violations))


def main() -> int:
    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "quadtree_hpp": "LandscapeEditor/src/TerrainQuadtree.hpp",
        "quadtree_cpp": "LandscapeEditor/src/TerrainQuadtree.cpp",
        "debug_hpp": "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.hpp",
        "debug_cpp": "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-cpu-quadtree-lod-selection.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("file_quadtree_hpp", lambda: f("quadtree_hpp")),
        ("file_quadtree_cpp", lambda: f("quadtree_cpp")),
        ("file_debug_hpp", lambda: f("debug_hpp")),
        ("file_debug_cpp", lambda: f("debug_cpp")),
        ("cmake_quadtree_cpp", lambda: require_contains(f("cmake"), r"TerrainQuadtree\.cpp", "TerrainQuadtree source in CMake")),
        ("cmake_quadtree_hpp", lambda: require_contains(f("cmake"), r"TerrainQuadtree\.hpp", "TerrainQuadtree header in CMake")),
        ("cmake_debug_cpp", lambda: require_contains(f("cmake"), r"TerrainQuadtreeDebugRenderer\.cpp", "TerrainQuadtreeDebugRenderer source in CMake")),
        ("cmake_debug_hpp", lambda: require_contains(f("cmake"), r"TerrainQuadtreeDebugRenderer\.hpp", "TerrainQuadtreeDebugRenderer header in CMake")),
        ("quadtree_desc", lambda: require_contains(f("quadtree_hpp"), r"struct\s+TerrainQuadtreeDesc.*MaxDepth\s*=\s*4.*Extent\s*=\s*20\.0f", "quadtree descriptor defaults")),
        ("quadtree_node", lambda: require_contains(f("quadtree_hpp"), r"struct\s+TerrainQuadtreeNode.*NodeIndex.*ParentIndex.*FirstChildIndex.*Level.*ChildMask.*MinXZ.*MaxXZ.*CenterXZ.*Radius", "quadtree node fields")),
        ("quadtree_selection", lambda: require_contains(f("quadtree_hpp"), r"struct\s+TerrainQuadtreeSelection.*SelectedNodeIndices.*MaxSelectedLevel", "quadtree selection fields")),
        ("quadtree_api", lambda: require_contains(f("quadtree_hpp"), r"Build\s*\(.*Select\s*\(.*GetNodes\s*\(.*GetMaxDepth\s*\(.*GetSelectedLeafCapacity", "quadtree public API")),
        ("quadtree_invalid_index", lambda: require_contains(f("quadtree_hpp"), r"InvalidNodeIndex\s*=\s*0xFFFFFFFFu", "invalid node index constant")),
        ("quadtree_stable_children", lambda: require_contains(f("quadtree_cpp"), r"SouthWest.*SouthEast.*NorthWest.*NorthEast", "stable child order comments or identifiers")),
        ("quadtree_contiguous_children", lambda: require_contains(f("quadtree_cpp"), r"FirstChildIndex\s*=.*m_Nodes\.size\s*\(\).*AddNode\s*\(\s*NodeIndex\s*,\s*ChildLevel\s*,\s*SouthWestMin.*AddNode\s*\(\s*NodeIndex\s*,\s*ChildLevel\s*,\s*SouthEastMin.*AddNode\s*\(\s*NodeIndex\s*,\s*ChildLevel\s*,\s*NorthWestMin.*AddNode\s*\(\s*NodeIndex\s*,\s*ChildLevel\s*,\s*NorthEastMin.*SubdivideNode\s*\(\s*FirstChildIndex\s*\+\s*Child\s*\)", "contiguous direct child node layout")),
        ("quadtree_xz_distance", lambda: require_contains(f("quadtree_cpp"), r"CameraPosition\.x.*CameraPosition\.z.*CenterXZ.*SplitDistance", "camera XZ distance LOD rule")),
        ("quadtree_split_rule", lambda: require_contains(f("quadtree_cpp"), r"2\.2f.*\(1u\s*<<\s*Node\.Level\)", "split-distance formula")),
        ("debug_vertex_format", lambda: require_contains(f("debug_cpp"), r"float3\s+Pos.*float3\s+Color", "quadtree debug vertex format")),
        ("debug_dynamic_buffer", lambda: require_contains(f("debug_cpp"), r"USAGE_DYNAMIC.*BIND_VERTEX_BUFFER.*CPU_ACCESS_WRITE", "dynamic quadtree debug vertex buffer")),
        ("debug_pso_key", lambda: require_contains(f("debug_cpp"), r"ForwardDebug\.TerrainQuadtree\.LineList\.v1", "quadtree debug PSO cache key")),
        ("debug_glsl_language", lambda: require_contains(f("debug_cpp"), r"SHADER_SOURCE_LANGUAGE_GLSL", "OpenGL GLSL debug overlay shader language")),
        ("debug_glsl_attribute", lambda: require_contains(f("debug_cpp"), r"layout\s*\(\s*location\s*=\s*0\s*\)", "OpenGL GLSL debug overlay attributes")),
        ("renderer_owns_quadtree", lambda: require_contains(f("renderer_hpp"), r"TerrainQuadtree\s+m_TerrainQuadtree.*TerrainQuadtreeSelection\s+m_TerrainQuadtreeSelection.*TerrainQuadtreeDebugRenderer\s+m_TerrainQuadtreeDebugRenderer", "ForwardRenderer quadtree members")),
        ("renderer_quadtree_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainQuadtreeNodeCount.*TerrainQuadtreeSelectedLeafCount.*TerrainQuadtreeMaxDepth.*TerrainQuadtreeMaxSelectedLevel", "ForwardRenderer quadtree stats")),
        ("renderer_selects_each_frame", lambda: require_contains(f("renderer_cpp"), r"m_TerrainQuadtree\.Select\s*\(\s*View\.CameraPosition\s*,\s*m_TerrainQuadtreeSelection\s*\)", "per-frame quadtree selection")),
        ("renderer_renders_overlay", lambda: require_contains(f("renderer_cpp"), r"m_ShowQuadtreeOverlay.*m_TerrainQuadtreeDebugRenderer\.Render", "quadtree overlay render toggle")),
        ("editor_overlay_toggle", lambda: require_contains(f("editor_cpp"), r"Checkbox\s*\(\s*\"Show quadtree overlay\".*SetShowQuadtreeOverlay", "ImGui quadtree overlay toggle")),
        ("editor_quadtree_stats", lambda: require_contains(f("editor_cpp"), r"Quadtree nodes.*Selected leaves.*Max selected LOD", "ImGui quadtree stats")),
        ("status_quadtree_done", lambda: require_contains(f("status"), r"CPU quadtree LOD selection", "quadtree status record")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record", "quadtree plan validation record")),
        ("no_pso_creation_in_render", lambda: require_no_pso_creation_in_render([
            "LandscapeEditor/src/ForwardRenderer.cpp",
            "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.cpp",
        ])),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("quadtree LOD verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("quadtree LOD verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
