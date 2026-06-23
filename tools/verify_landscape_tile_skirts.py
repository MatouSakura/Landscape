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
        "queue": "LandscapeEditor/src/RenderQueue.hpp",
        "terrain_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "terrain_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-terrain-tile-skirts.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("draw_region_skirt_fields", lambda: require_contains(f("queue"), r"TerrainDrawRegion.*MainNumIndices.*SkirtIndexCount", "main and skirt index fields on TerrainDrawRegion")),
        ("skirt_depth_constant", lambda: require_contains(f("terrain_cpp"), r"SkirtDepth\s*=\s*1\.25f", "fixed terrain skirt depth")),
        ("skirt_generation_helper", lambda: require_contains(f("terrain_cpp"), r"AppendTileSkirts\s*\(.*West.*East.*South.*North", "four-edge tile skirt generation helper")),
        ("skirt_bottom_vertices", lambda: require_contains(f("terrain_cpp"), r"SkirtDepth.*TopPos\.y\s*-\s*SkirtDepth", "skirt bottom vertex offset")),
        ("main_indices_recorded_before_skirts", lambda: require_contains(f("terrain_cpp"), r"Region\.MainNumIndices\s*=.*Region\.FirstIndexLocation.*AppendTileSkirts.*Region\.SkirtIndexCount", "main and skirt index counts recorded separately")),
        ("draw_toggle_uses_main_or_total", lambda: require_contains(f("terrain_cpp"), r"m_EnableSkirts\s*\?\s*Region\.NumIndices\s*:\s*Region\.MainNumIndices", "draw path switches between main-only and skirt-enabled indices")),
        ("terrain_skirt_api", lambda: require_contains(f("terrain_hpp"), r"SetEnableSkirts.*GetEnableSkirts.*GetSkirtDepth.*GetPackedTileSkirtVertexCount.*GetPackedTileSkirtIndexCount", "terrain skirt API and stats")),
        ("terrain_rendered_index_stats", lambda: require_contains(f("terrain_hpp"), r"GetLastRenderedTriangleCount.*m_LastRenderedIndexCount", "rendered index count drives triangle stats")),
        ("renderer_skirt_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainSkirtsEnabled.*TerrainSkirtDepth.*TerrainSkirtVertexCount.*TerrainSkirtIndexCount", "ForwardRenderer skirt stats")),
        ("renderer_skirt_toggle", lambda: require_contains(f("renderer_hpp") + f("renderer_cpp"), r"SetTerrainSkirtsEnabled.*SetEnableSkirts", "ForwardRenderer terrain skirt toggle")),
        ("editor_skirt_toggle", lambda: require_contains(f("editor_hpp") + f("editor_cpp"), r"m_EnableTerrainSkirts.*Enable terrain skirts", "ImGui terrain skirt toggle")),
        ("editor_skirt_stats", lambda: require_contains(f("editor_cpp"), r"Skirt depth.*Skirt vertices.*Skirt indices", "ImGui terrain skirt stats")),
        ("status_record", lambda: require_contains(f("status"), r"terrain tile skirts", "status record for terrain tile skirts")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "terrain tile skirts validation record")),
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
        print("terrain tile skirts verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("terrain tile skirts verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
