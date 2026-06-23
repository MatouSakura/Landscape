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
        "index_hpp": "LandscapeEditor/src/TerrainLODIndexStitching.hpp",
        "index_cpp": "LandscapeEditor/src/TerrainLODIndexStitching.cpp",
        "queue_hpp": "LandscapeEditor/src/RenderQueue.hpp",
        "patch_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "patch_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-lod-index-stitching-v1.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    def combined(*keys: str) -> str:
        return "".join(f(key) for key in keys)

    checks = [
        ("cmake_cpp", lambda: require_contains(f("cmake"), r"TerrainLODIndexStitching\.cpp", "TerrainLODIndexStitching source in CMake")),
        ("cmake_hpp", lambda: require_contains(f("cmake"), r"TerrainLODIndexStitching\.hpp", "TerrainLODIndexStitching header in CMake")),
        ("edge_mask_type", lambda: require_contains(combined("index_hpp", "index_cpp"), r"enum\s+class\s+TerrainLODStitchEdgeMask.*West.*East.*South.*North", "stitch edge mask enum")),
        ("stitched_region_type", lambda: require_contains(combined("index_hpp", "index_cpp"), r"struct\s+TerrainLODStitchedDrawRegion.*NodeIndex.*EdgeMask.*FirstIndexLocation.*NumIndices", "stitched draw region type")),
        ("index_stats_type", lambda: require_contains(combined("index_hpp", "index_cpp"), r"TerrainLODIndexStitchingStats.*StitchedNodeCount.*StitchedEdgeCount.*GeneratedIndexCount.*MaxStitchRatio", "LOD index stitching stats")),
        ("build_consumes_seams", lambda: require_contains(combined("index_hpp", "index_cpp"), r"Build\s*\([^;]*TerrainLODStitching", "index stitching build consumes seam plan")),
        ("fine_node_masks", lambda: require_contains(f("index_cpp"), r"FineNodeIndex.*EdgeMask.*TerrainLODSeamSide", "fine-node edge masks from seam sides")),
        ("stitch_ratio_stats", lambda: require_contains(f("index_cpp"), r"MaxStitchRatio.*StitchRatio.*GeneratedIndexCount", "index stitching stats generated from seam ratios")),
        ("queue_region_fields", lambda: require_contains(f("queue_hpp"), r"UseStitchedIndexBuffer.*StitchedFirstIndexLocation.*StitchedNumIndices", "terrain draw region stitched index fields")),
        ("patch_dynamic_buffer", lambda: require_contains(combined("patch_hpp", "patch_cpp"), r"m_pStitchedIndexBuffer.*USAGE_DYNAMIC.*BIND_INDEX_BUFFER.*CPU_ACCESS_WRITE", "dynamic stitched index buffer")),
        ("patch_prepare_api", lambda: require_contains(combined("patch_hpp", "patch_cpp"), r"PrepareLODIndexStitching\s*\([^)]*TerrainLODIndexStitching", "TerrainPatchRenderer prepare LOD index stitching API")),
        ("patch_map_indices", lambda: require_contains(f("patch_cpp"), r"MapHelper<Uint32>.*m_pStitchedIndexBuffer.*MAP_WRITE.*MAP_FLAG_DISCARD", "dynamic stitched index buffer map/update")),
        ("patch_generates_stitch_band", lambda: require_contains(f("patch_cpp"), r"AppendStitchedEdgeBand.*TerrainLODStitchEdgeMask.*StitchRatio", "stitched edge-band index generation")),
        ("patch_draw_uses_stitched_buffer", lambda: require_contains(f("patch_cpp"), r"UseStitchedIndexBuffer.*m_pStitchedIndexBuffer.*StitchedFirstIndexLocation.*StitchedNumIndices", "draw path uses stitched index buffer regions")),
        ("renderer_member", lambda: require_contains(f("renderer_hpp"), r"TerrainLODIndexStitching\s+m_TerrainLODIndexStitching", "ForwardRenderer owns LOD index stitching")),
        ("renderer_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainLODIndexStitchingNodeCount.*TerrainLODIndexStitchingEdgeCount.*TerrainLODIndexStitchingIndexCount.*TerrainLODIndexStitchingMaxRatio", "ForwardRenderer LOD index stitching stats")),
        ("renderer_builds_index_stitching", lambda: require_contains(f("renderer_cpp"), r"m_TerrainLODIndexStitching\.Build\s*\([^;]*m_TerrainLODStitching", "ForwardRenderer builds index stitching from seam plan")),
        ("renderer_prepares_buffer", lambda: require_contains(f("renderer_cpp"), r"PrepareLODIndexStitching\s*\([^;]*m_TerrainLODIndexStitching", "ForwardRenderer prepares dynamic stitched index buffer")),
        ("renderer_submits_stitched_regions", lambda: require_contains(f("renderer_cpp"), r"ApplyLODIndexStitching.*m_TerrainLODIndexStitching", "ForwardRenderer submits stitched draw regions")),
        ("editor_stats", lambda: require_contains(f("editor_cpp"), r"LOD index stitched nodes.*LOD index stitched indices", "ImGui LOD index stitching stats")),
        ("status_record", lambda: require_contains(f("status"), r"LOD index stitching v1", "status record for LOD index stitching v1")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "LOD index stitching validation record")),
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
        print("LOD index stitching verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("LOD index stitching verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
