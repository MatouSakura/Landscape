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
        "queue": "LandscapeEditor/src/RenderQueue.hpp",
        "terrain_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "terrain_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "selected_verify": "tools/verify_landscape_selected_leaf_render_items.py",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-packed-tile-mesh-cache.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("draw_region_mesh_fields", lambda: require_contains(f("queue"), r"TerrainDrawRegion.*BaseVertex.*FirstIndexLocation.*NumIndices", "packed mesh draw fields on TerrainDrawRegion")),
        ("terrain_initialize_nodes", lambda: require_contains(f("terrain_hpp"), r"Initialize\s*\([^;]*const\s+std::vector<\s*TerrainQuadtreeNode\s*>&\s+QuadtreeNodes", "terrain renderer initialization accepts quadtree nodes")),
        ("tile_mesh_range_type", lambda: require_contains(f("terrain_hpp"), r"struct\s+TerrainTileMeshRange.*TerrainDrawRegion\s+Region.*VertexCount", "terrain tile mesh range type")),
        ("tile_mesh_cache_members", lambda: require_contains(f("terrain_hpp"), r"m_TileMeshRanges.*m_PackedTileVertexCount.*m_PackedTileIndexCount", "packed tile mesh cache members")),
        ("tile_mesh_cache_stats", lambda: require_contains(f("terrain_hpp"), r"GetTileMeshCount.*GetPackedTileVertexCount.*GetPackedTileIndexCount", "packed tile mesh cache stats")),
        ("build_tile_mesh_cache_function", lambda: require_contains(f("terrain_cpp"), r"BuildPackedTileMeshCache\s*\([^)]*QuadtreeNodes", "packed tile mesh cache builder")),
        ("build_per_node_tiles", lambda: require_contains(f("terrain_cpp"), r"for\s*\([^)]*TerrainQuadtreeNode.*QuadtreeNodes.*BaseVertex.*FirstIndexLocation.*NumIndices.*m_TileMeshRanges", "per-node packed tile mesh ranges")),
        ("local_tile_indices", lambda: require_contains(f("terrain_cpp"), r"LocalRowStride.*LocalI0.*LocalI1.*LocalI2.*LocalI3", "local tile index generation")),
        ("single_draw_uses_mesh_range", lambda: require_contains(f("terrain_cpp"), r"GetTerrainRegionDrawIndexCount.*Region\.NumIndices.*Region\.MainNumIndices.*FirstIndexLocation\s*=.*Region\.FirstIndexLocation.*DrawAttrs\.NumIndices\s*=\s*DrawIndexCount.*DrawAttrs\.FirstIndexLocation\s*=\s*FirstIndexLocation.*DrawAttrs\.BaseVertex\s*=\s*Region\.BaseVertex", "single indexed draw uses packed mesh range")),
        ("row_draw_removed", lambda: require_not_contains(f("terrain_cpp"), r"DrawRegionIndexed", "row-based region draw helper")),
        ("renderer_initializes_quadtree_first", lambda: require_contains(f("renderer_cpp"), r"m_TerrainQuadtree\.Build.*m_TerrainPatchRenderer\.Initialize\s*\([^;]*m_TerrainQuadtree\.GetNodes\s*\(\s*\)", "ForwardRenderer builds quadtree before terrain mesh cache")),
        ("renderer_tile_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainTileMeshCount.*TerrainPackedVertexCount.*TerrainPackedIndexCount", "ForwardRenderer packed tile mesh stats")),
        ("ui_tile_stats", lambda: require_contains(f("editor_cpp"), r"Tile meshes.*Packed tile vertices.*Packed tile indices", "ImGui packed tile mesh stats")),
        ("selected_verify_updated", lambda: require_contains(f("selected_verify"), r"NumIndices.*FirstIndexLocation.*BaseVertex", "selected leaf verification accepts packed tile mesh draws")),
        ("status_record", lambda: require_contains(f("status"), r"packed tile mesh cache", "status record for packed tile mesh cache")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "packed tile mesh validation record")),
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
        print("packed tile mesh cache verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("packed tile mesh cache verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
