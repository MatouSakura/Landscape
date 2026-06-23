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
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "stitch_hpp": "LandscapeEditor/src/TerrainLODStitching.hpp",
        "stitch_cpp": "LandscapeEditor/src/TerrainLODStitching.cpp",
        "debug_hpp": "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.hpp",
        "debug_cpp": "LandscapeEditor/src/TerrainQuadtreeDebugRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "transition_verify": "tools/verify_landscape_lod_transition_diagnostics.py",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-lod-stitching-seam-plan.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    debug_text = f("debug_hpp") + f("debug_cpp")

    checks = [
        ("cmake_cpp", lambda: require_contains(f("cmake"), r"TerrainLODStitching\.cpp", "TerrainLODStitching source in CMake")),
        ("cmake_hpp", lambda: require_contains(f("cmake"), r"TerrainLODStitching\.hpp", "TerrainLODStitching header in CMake")),
        ("seam_side_enum", lambda: require_contains(f("stitch_hpp"), r"enum\s+class\s+TerrainLODSeamSide.*West.*East.*South.*North", "seam side enum")),
        ("seam_edge_type", lambda: require_contains(f("stitch_hpp"), r"struct\s+TerrainLODSeamEdge.*FineNodeIndex.*CoarseNodeIndex.*FineSide.*StartXZ.*EndXZ.*LODDelta.*StitchRatio.*Length", "terrain LOD seam edge type")),
        ("seam_stats_type", lambda: require_contains(f("stitch_hpp"), r"TerrainLODStitchingStats.*SeamEdgeCount.*MaxLODDelta.*MaxStitchRatio.*TotalSeamLength", "terrain LOD stitching stats")),
        ("stitching_api", lambda: require_contains(f("stitch_hpp"), r"class\s+TerrainLODStitching.*Build\s*\([^;]*TerrainQuadtreeSelection.*GetSeams.*GetStats", "TerrainLODStitching public API")),
        ("pairwise_seam_build", lambda: require_contains(f("stitch_cpp"), r"TryBuildSeamEdge.*Lhs\.Level\s*==\s*Rhs\.Level.*FineNode.*CoarseNode.*StitchRatio", "pairwise fine-to-coarse seam builder")),
        ("seam_side_detection", lambda: require_contains(f("stitch_cpp"), r"TerrainLODSeamSide::East.*TerrainLODSeamSide::West.*TerrainLODSeamSide::North.*TerrainLODSeamSide::South", "edge-side detection for all four directions")),
        ("stitching_build_selection", lambda: require_contains(f("stitch_cpp"), r"TerrainLODStitching::Build.*SelectedNodeIndices.*m_Seams\.push_back.*m_Stats\.SeamEdgeCount", "TerrainLODStitching builds from selected leaves")),
        ("renderer_member", lambda: require_contains(f("renderer_hpp"), r"TerrainLODStitching\s+m_TerrainLODStitching", "ForwardRenderer owns LOD stitching")),
        ("renderer_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainLODStitchingSeamEdgeCount.*TerrainLODStitchingMaxDelta.*TerrainLODStitchingMaxRatio.*TerrainLODStitchingTotalLength", "ForwardRenderer LOD stitching stats")),
        ("renderer_builds_stitching", lambda: require_contains(f("renderer_cpp"), r"m_TerrainLODStitching\.Build\s*\([^;]*m_VisibleTerrainQuadtreeSelection", "ForwardRenderer builds seam plan from visible selection")),
        ("renderer_updates_stitching_stats", lambda: require_contains(f("renderer_cpp"), r"GetStats\s*\(\s*\).*SeamEdgeCount.*MaxLODDelta.*MaxStitchRatio.*TotalSeamLength", "ForwardRenderer copies seam stats")),
        ("debug_consumes_stitching", lambda: require_contains(debug_text, r"TerrainLODStitching.*AppendLODTransitionEdges\s*\([^)]*TerrainLODStitching", "debug overlay consumes seam plan")),
        ("debug_no_pairwise_selected_transition", lambda: require_contains(f("debug_cpp"), r"for\s*\(\s*const\s+TerrainLODSeamEdge&\s+Seam\s*:\s*Stitching\.GetSeams", "debug transition lines iterate seam plan")),
        ("editor_stitching_stats", lambda: require_contains(f("editor_cpp"), r"LOD stitching seams.*LOD stitching max delta.*LOD stitching length", "ImGui LOD stitching stats")),
        ("transition_verify_updated", lambda: require_contains(f("transition_verify"), r"TerrainLODStitching.*AppendLODTransitionEdges", "LOD transition verifier updated for seam plan")),
        ("status_record", lambda: require_contains(f("status"), r"LOD stitching seam plan", "status record for LOD stitching seam plan")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "LOD stitching validation record")),
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
        print("LOD stitching verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("LOD stitching verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
