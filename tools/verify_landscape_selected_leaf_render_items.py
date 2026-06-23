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
        "shadow_hpp": "LandscapeEditor/src/ShadowRenderer.hpp",
        "shadow_cpp": "LandscapeEditor/src/ShadowRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-selected-leaf-terrain-render-items.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("terrain_draw_region_type", lambda: require_contains(f("queue"), r"struct\s+TerrainDrawRegion.*NodeIndex.*Level.*MinXZ.*MaxXZ.*FirstCellX.*FirstCellZ.*CellCountX.*CellCountZ", "terrain draw region fields")),
        ("terrain_leaf_kind", lambda: require_contains(f("queue"), r"RenderItemKind.*TerrainLeaf", "TerrainLeaf render item kind")),
        ("render_item_region", lambda: require_contains(f("queue"), r"struct\s+RenderItem.*TerrainDrawRegion\s+TerrainRegion", "terrain draw region stored on render item")),
        ("queue_add_terrain_leaf", lambda: require_contains(f("queue"), r"AddTerrainLeaf\s*\(\s*const\s+TerrainDrawRegion&.*RenderItemKind::TerrainLeaf", "AddTerrainLeaf queue submission")),
        ("renderer_submits_selected_leaves", lambda: require_contains(f("renderer_cpp"), r"SelectedNodeIndices.*BuildDrawRegion.*AddTerrainLeaf", "ForwardRenderer submits selected leaves as opaque render items")),
        ("renderer_no_single_patch_submit", lambda: require_contains(f("renderer_cpp"), r"m_RenderQueue\.AddTerrainLeaf", "ForwardRenderer uses AddTerrainLeaf")),
        ("terrain_build_draw_region", lambda: require_contains(f("terrain_hpp"), r"BuildDrawRegion\s*\(\s*const\s+TerrainQuadtreeNode&", "TerrainPatchRenderer draw-region builder API")),
        ("terrain_render_region_signature", lambda: require_contains(f("terrain_hpp"), r"Render\s*\([^;]*const\s+TerrainDrawRegion&", "TerrainPatchRenderer forward render region API")),
        ("terrain_shadow_region_signature", lambda: require_contains(f("terrain_hpp"), r"RenderShadow\s*\([^;]*const\s+TerrainDrawRegion&", "TerrainPatchRenderer shadow render region API")),
        ("terrain_first_index_location", lambda: require_contains(f("terrain_cpp"), r"NumIndices\s*=\s*Region\.NumIndices.*FirstIndexLocation\s*=\s*Region\.FirstIndexLocation.*BaseVertex\s*=\s*Region\.BaseVertex", "packed indexed region drawing")),
        ("terrain_region_cell_mapping", lambda: require_contains(f("terrain_cpp"), r"Node\.MinXZ.*Node\.MaxXZ.*CellCount.*FirstCellX.*CellCountX", "quadtree node bounds mapped to terrain cells")),
        ("shadow_uses_opaque_items", lambda: require_contains(f("shadow_hpp"), r"Render\s*\([^;]*const\s+std::vector<RenderItem>&", "ShadowRenderer accepts terrain render items")),
        ("shadow_renders_terrain_leaf_regions", lambda: require_contains(f("shadow_cpp"), r"RenderItemKind::TerrainLeaf.*RenderShadow\s*\([^;]*Item\.TerrainRegion", "ShadowRenderer draws terrain leaf regions")),
        ("forward_draws_terrain_leaf_regions", lambda: require_contains(f("renderer_cpp"), r"RenderItemKind::TerrainLeaf.*m_TerrainPatchRenderer\.Render\s*\([^;]*Item\.TerrainRegion", "ForwardRenderer draws terrain leaf regions")),
        ("stats_render_items", lambda: require_contains(f("renderer_hpp"), r"TerrainRenderItemCount.*TerrainRenderedCellCount.*TerrainForwardDrawCallCount.*TerrainShadowDrawCallCount", "terrain render item stats")),
        ("ui_render_item_stats", lambda: require_contains(f("editor_cpp"), r"Terrain render items.*Terrain rendered cells.*Terrain draw calls", "ImGui terrain render item stats")),
        ("status_record", lambda: require_contains(f("status"), r"selected leaf terrain render items", "status record for selected leaf render items")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record", "selected leaf render item plan validation record")),
        ("no_pso_creation_in_render", lambda: require_no_pso_creation_in_render([
            "LandscapeEditor/src/ForwardRenderer.cpp",
            "LandscapeEditor/src/TerrainPatchRenderer.cpp",
            "LandscapeEditor/src/ShadowRenderer.cpp",
        ])),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("selected leaf terrain render item verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("selected leaf terrain render item verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
