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
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-terrain-frustum-culling.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("advanced_math_include", lambda: require_contains(f("renderer_cpp"), r"#include\s+\"AdvancedMath\.hpp\"", "AdvancedMath frustum utilities include")),
        ("frustum_culling_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainFrustumCullingEnabled.*TerrainQuadtreeCandidateLeafCount.*TerrainQuadtreeVisibleLeafCount.*TerrainFrustumCulledLeafCount", "frustum culling stats fields")),
        ("visible_selection_member", lambda: require_contains(f("renderer_hpp"), r"m_VisibleTerrainQuadtreeSelection", "visible quadtree selection member")),
        ("frustum_toggle", lambda: require_contains(renderer_text, r"SetTerrainFrustumCullingEnabled.*m_EnableTerrainFrustumCulling", "ForwardRenderer frustum culling toggle")),
        ("node_bounds_helper", lambda: require_contains(f("renderer_cpp"), r"BuildTerrainNodeBounds\s*\([^)]*TerrainQuadtreeNode.*MinHeight.*MaxHeight.*SkirtDepth.*BoundBox", "terrain node bounds helper")),
        ("frustum_extraction", lambda: require_contains(f("renderer_cpp"), r"ExtractViewFrustumPlanesFromMatrix\s*\(\s*View\.ViewProj\s*,\s*Frustum\s*,\s*View\.IsGL\s*\)", "view frustum extraction from RenderView")),
        ("box_visibility_test", lambda: require_contains(f("renderer_cpp"), r"GetBoxVisibility\s*\(\s*Frustum\s*,\s*Bounds\s*\)\s*!=\s*BoxVisibility::Invisible", "AABB visibility test")),
        ("visible_selection_build", lambda: require_contains(f("renderer_cpp"), r"BuildVisibleTerrainSelection.*OutVisibleSelection\.SelectedNodeIndices\.clear.*EnableFrustumCulling.*SelectedNodeIndices\.push_back", "visible selection is rebuilt from candidates")),
        ("render_uses_visible_selection", lambda: require_contains(f("renderer_cpp"), r"for\s*\(\s*Uint32\s+NodeIndex\s*:\s*m_VisibleTerrainQuadtreeSelection\.SelectedNodeIndices\s*\).*m_RenderQueue\.AddTerrainLeaf", "render queue uses visible selection")),
        ("debug_uses_visible_selection", lambda: require_contains(f("renderer_cpp"), r"m_TerrainQuadtreeDebugRenderer\.Render\s*\([^;]*m_VisibleTerrainQuadtreeSelection", "debug overlay uses visible selection")),
        ("renderer_updates_culling_stats", lambda: require_contains(f("renderer_cpp"), r"TerrainQuadtreeCandidateLeafCount.*TerrainQuadtreeVisibleLeafCount.*TerrainFrustumCulledLeafCount", "ForwardRenderer updates frustum culling stats")),
        ("editor_toggle_state", lambda: require_contains(editor_text, r"m_EnableTerrainFrustumCulling.*Enable terrain frustum culling", "ImGui frustum culling toggle")),
        ("editor_stats", lambda: require_contains(f("editor_cpp"), r"Candidate leaves.*Visible leaves.*Frustum culled leaves", "ImGui frustum culling stats")),
        ("status_record", lambda: require_contains(f("status"), r"frustum culling", "status record for terrain frustum culling")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "terrain frustum culling validation record")),
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
        print("terrain frustum culling verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("terrain frustum culling verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
