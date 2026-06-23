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
        "quadtree_hpp": "LandscapeEditor/src/TerrainQuadtree.hpp",
        "quadtree_cpp": "LandscapeEditor/src/TerrainQuadtree.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "quadtree_verify": "tools/verify_landscape_quadtree_lod.py",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-ue-style-component-lod-policy.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    quadtree_text = f("quadtree_hpp") + f("quadtree_cpp")
    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("lod_policy_type", lambda: require_contains(f("quadtree_hpp"), r"struct\s+TerrainComponentLODPolicy.*SplitDistanceScale.*MaxSelectedLevel", "TerrainComponentLODPolicy type")),
        ("selection_min_max", lambda: require_contains(f("quadtree_hpp"), r"TerrainQuadtreeSelection.*MinSelectedLevel.*MaxSelectedLevel", "selection min/max LOD levels")),
        ("lod_policy_api", lambda: require_contains(f("quadtree_hpp"), r"SetLODPolicy.*GetLODPolicy.*GetMaxSelectableLevel.*ComputeSplitDistance.*GetComponentWorldSize", "quadtree LOD policy API")),
        ("policy_clamp", lambda: require_contains(f("quadtree_cpp"), r"ClampLODPolicy.*SplitDistanceScale.*MaxSelectedLevel", "LOD policy clamping")),
        ("policy_split_distance", lambda: require_contains(f("quadtree_cpp"), r"ComputeSplitDistance\s*\([^)]*Level[^)]*\).*SplitDistanceScale.*1u\s*<<\s*SafeLevel", "policy split distance formula")),
        ("policy_stops_split", lambda: require_contains(f("quadtree_cpp"), r"Node\.Level\s*<\s*GetMaxSelectableLevel\s*\(\s*\).*DistanceXZ\s*<\s*SplitDistance", "selection stops at max selectable LOD")),
        ("component_world_size", lambda: require_contains(f("quadtree_cpp"), r"GetComponentWorldSize\s*\([^)]*Level[^)]*\).*2\.0f\s*\*\s*m_Desc\.Extent.*1u\s*<<\s*SafeLevel", "component world size helper")),
        ("hardcoded_split_removed", lambda: require_not_contains(f("quadtree_cpp"), r"2\.2f\s*/\s*static_cast<float>\s*\(\s*1u\s*<<\s*Node\.Level\s*\)", "old hard-coded split-distance expression")),
        ("renderer_policy_controls", lambda: require_contains(f("renderer_hpp"), r"SetTerrainLODDistanceScale.*GetTerrainLODDistanceScale.*SetTerrainMaxSelectedLODLevel.*GetTerrainMaxSelectedLODLevel", "ForwardRenderer LOD policy controls")),
        ("renderer_policy_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainLODDistanceScale.*TerrainMaxSelectableLODLevel.*TerrainQuadtreeMinSelectedLevel.*TerrainRootComponentWorldSize.*TerrainFineComponentWorldSize.*TerrainSelectedMinComponentWorldSize.*TerrainSelectedMaxComponentWorldSize", "ForwardRenderer component LOD stats")),
        ("renderer_updates_policy_stats", lambda: require_contains(f("renderer_cpp"), r"UpdateTerrainComponentLODStats.*GetLODPolicy.*GetComponentWorldSize.*TerrainSelectedMinComponentWorldSize.*TerrainSelectedMaxComponentWorldSize", "ForwardRenderer updates component LOD stats")),
        ("editor_lod_state", lambda: require_contains(f("editor_hpp"), r"m_TerrainLODDistanceScale.*m_TerrainMaxSelectedLODLevel", "LandscapeEditor LOD policy UI state")),
        ("editor_lod_ui", lambda: require_contains(f("editor_cpp"), r"SliderFloat\s*\(\s*\"LOD distance scale\".*SliderInt\s*\(\s*\"Max terrain LOD\".*Component size", "ImGui LOD policy controls and stats")),
        ("quadtree_verify_updated", lambda: require_contains(f("quadtree_verify"), r"ComputeSplitDistance.*SplitDistanceScale.*GetMaxSelectableLevel", "older quadtree verifier accepts policy split rule")),
        ("status_record", lambda: require_contains(f("status"), r"UE-style component LOD policy", "status record for component LOD policy")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "component LOD policy validation record")),
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
        print("component LOD policy verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("component LOD policy verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
