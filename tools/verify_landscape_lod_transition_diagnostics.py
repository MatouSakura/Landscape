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
    pattern = re.compile(r"\bvoid\s+([A-Za-z0-9_]+::Render(?:Shadow)?)\s*\([^)]*\)\s*\{", re.MULTILINE)
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
        raise AssertionError("PSO creation/cache lookup found in render path: " + ", ".join(violations))


def main() -> int:
    files = {
        "debug_hpp": "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.hpp",
        "debug_cpp": "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.cpp",
        "stitch_hpp": "LandscapeEditor/src/TerrainLODStitching.hpp",
        "stitch_cpp": "LandscapeEditor/src/TerrainLODStitching.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-lod-transition-diagnostics.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    debug_text = f("debug_hpp") + f("debug_cpp")
    stitch_text = f("stitch_hpp") + f("stitch_cpp")
    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("debug_stats_type", lambda: require_contains(debug_text, r"TerrainQuadtreeDebugStats.*LeafBoundLineCount.*SkirtEdgeCount.*LODTransitionEdgeCount.*LineVertexCount", "quadtree debug stats type")),
        ("debug_toggles", lambda: require_contains(debug_text, r"SetShowSkirtEdges.*SetShowLODTransitionEdges.*GetStats", "quadtree diagnostic toggles and stats accessor")),
        ("skirt_edge_generation", lambda: require_contains(f("debug_cpp"), r"AppendSkirtEdges\s*\([^)]*TerrainQuadtreeNode.*SkirtEdgeColor", "selected leaf skirt edge generation")),
        ("transition_edge_generation", lambda: require_contains(f("debug_cpp"), r"TerrainLODStitching.*AppendLODTransitionEdges\s*\([^)]*TerrainLODStitching.*LODTransitionColor", "LOD transition edge generation from seam plan")),
        ("edge_overlap_helper", lambda: require_contains(stitch_text, r"TryBuildSeamEdge.*Lhs\.Level\s*==\s*Rhs\.Level.*OverlapMin.*OverlapMax", "adjacent mixed-LOD overlap detection")),
        ("render_appends_diagnostics", lambda: require_contains(f("debug_cpp"), r"m_ShowSkirtEdges.*AppendSkirtEdges.*m_ShowLODTransitionEdges.*AppendLODTransitionEdges", "render appends optional skirt and LOD transition diagnostics")),
        ("renderer_stats", lambda: require_contains(renderer_text, r"TerrainDebugLeafBoundLineCount.*TerrainDebugSkirtEdgeCount.*TerrainDebugLODTransitionEdgeCount.*TerrainDebugLineVertexCount", "ForwardRenderer debug diagnostic stats")),
        ("renderer_toggles", lambda: require_contains(renderer_text, r"SetShowSkirtEdgeOverlay.*SetShowLODTransitionOverlay", "ForwardRenderer diagnostic overlay toggles")),
        ("renderer_updates_debug_stats", lambda: require_contains(f("renderer_cpp"), r"GetStats\s*\(\s*\).*LeafBoundLineCount.*SkirtEdgeCount.*LODTransitionEdgeCount.*LineVertexCount", "ForwardRenderer copies quadtree debug stats")),
        ("editor_skirt_toggle", lambda: require_contains(editor_text, r"m_ShowSkirtEdgeOverlay", "skirt edge overlay state") or require_contains(editor_text, r"Show skirt edge overlay", "skirt edge overlay checkbox")),
        ("editor_lod_transition_toggle", lambda: require_contains(editor_text, r"m_ShowLODTransitionOverlay", "LOD transition overlay state") or require_contains(editor_text, r"Show LOD transition overlay", "LOD transition overlay checkbox")),
        ("editor_stats", lambda: require_contains(f("editor_cpp"), r"Debug leaf bound lines.*Debug skirt edges.*Debug LOD transition edges.*Debug line vertices", "ImGui diagnostic stats")),
        ("status_record", lambda: require_contains(f("status"), r"LOD transition diagnostics", "status record for LOD transition diagnostics")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "LOD transition diagnostics validation record")),
        ("no_pso_creation_in_render", lambda: require_no_pso_creation_in_render([
            "LandscapeEditor/src/ForwardRenderer.cpp",
            "LandscapeEditor/src/TerrainPatchRenderer.cpp",
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
        print("LOD transition diagnostics verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("LOD transition diagnostics verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
