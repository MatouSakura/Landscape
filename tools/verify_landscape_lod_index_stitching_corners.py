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
        "index_hpp": "LandscapeEditor/src/TerrainLODIndexStitching.hpp",
        "index_cpp": "LandscapeEditor/src/TerrainLODIndexStitching.cpp",
        "patch_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-lod-index-stitching-corner-hardening.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")
    index_text = f("index_hpp") + f("index_cpp")

    checks = [
        ("corner_stats_type", lambda: require_contains(index_text, r"TerrainLODIndexStitchingStats.*StitchedCornerCount.*CornerPatchIndexCount", "corner stats on LOD index stitching stats")),
        ("corner_counter", lambda: require_contains(f("index_cpp"), r"CountStitchedCorners.*TerrainLODStitchEdgeMask::West.*TerrainLODStitchEdgeMask::East.*TerrainLODStitchEdgeMask::South.*TerrainLODStitchEdgeMask::North", "stitched corner counter for all edge combinations")),
        ("edge_band_clipping", lambda: require_contains(f("patch_cpp"), r"AppendStitchedEdgeBand\s*\([^)]*StartSample[^)]*EndSample.*BuildStitchStops\s*\([^)]*StartSample[^)]*EndSample", "stitched edge band sample clipping")),
        ("corner_patch_generation", lambda: require_contains(f("patch_cpp"), r"AppendStitchedCornerPatch.*StitchWest.*StitchEast.*StitchSouth.*StitchNorth", "corner patch generation")),
        ("corner_patch_indices", lambda: require_contains(f("patch_cpp"), r"CornerPatchIndexCount.*AppendStitchedCornerPatch", "corner patch index stats")),
        ("edge_corner_skip", lambda: require_contains(f("patch_cpp"), r"VerticalStartSample.*StitchSouth.*VerticalEndSample.*StitchNorth.*HorizontalStartSample.*StitchWest.*HorizontalEndSample.*StitchEast.*AppendStitchedEdgeBand", "edge bands clipped when adjacent stitched corners exist")),
        ("renderer_enable_field", lambda: require_contains(f("renderer_hpp"), r"m_EnableTerrainLODIndexStitching", "ForwardRenderer stores LOD index stitching enable state")),
        ("renderer_enable_api", lambda: require_contains(f("renderer_hpp"), r"SetTerrainLODIndexStitchingEnabled.*GetTerrainLODIndexStitchingEnabled", "ForwardRenderer LOD index stitching toggle API")),
        ("renderer_toggle_path", lambda: require_contains(f("renderer_cpp"), r"m_EnableTerrainLODIndexStitching.*m_TerrainLODIndexStitching\.Build.*PrepareLODIndexStitching", "ForwardRenderer gates dynamic index stitching path")),
        ("renderer_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainLODIndexStitchingEnabled.*TerrainLODIndexStitchingCornerCount.*TerrainLODIndexStitchingCornerIndexCount", "ForwardRenderer corner/toggle stats")),
        ("editor_toggle", lambda: require_contains(editor_text, r"m_EnableTerrainLODIndexStitching.*Enable LOD index stitching.*SetTerrainLODIndexStitchingEnabled", "ImGui LOD index stitching toggle")),
        ("editor_stats", lambda: require_contains(f("editor_cpp"), r"LOD index stitched corners.*LOD index corner indices", "ImGui LOD index corner stats")),
        ("status_record", lambda: require_contains(f("status"), r"LOD index stitching corner hardening", "status record for corner hardening")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "corner hardening validation record")),
        ("no_pso_creation_in_render", lambda: require_no_pso_creation_in_render([
            "LandscapeEditor/src/ForwardRenderer.cpp",
            "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        ])),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("LOD index stitching corner verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("LOD index stitching corner verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
