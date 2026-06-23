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
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-lod-tile-sampling.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("draw_region_lod_fields", lambda: require_contains(f("queue"), r"TerrainDrawRegion.*LODSampleStep.*MeshCellCountX.*MeshCellCountZ.*MeshSampleCountX.*MeshSampleCountZ", "LOD sample step and mesh cell fields on TerrainDrawRegion")),
        ("lod_step_helper", lambda: require_contains(f("terrain_cpp"), r"ComputeTerrainLODSampleStep\s*\([^)]*NodeLevel[^)]*MaxQuadtreeLevel[^)]*\).*MaxQuadtreeLevel\s*-\s*NodeLevel.*1u\s*<<", "LOD sample step helper based on max quadtree level and node level")),
        ("sample_coordinate_helper", lambda: require_contains(f("terrain_cpp"), r"BuildSampledCellCoordinates\s*\([^)]*FirstCell[^)]*CellCount[^)]*LODSampleStep[^)]*\).*push_back\s*\(\s*EndCell", "sample coordinate helper that includes end boundary")),
        ("region_records_lod_sampling", lambda: require_contains(f("terrain_cpp"), r"Region\.LODSampleStep\s*=.*ComputeTerrainLODSampleStep.*Region\.MeshSampleCountX\s*=.*SampleCellXs\.size.*Region\.MeshCellCountX\s*=.*Region\.MeshSampleCountX", "packed mesh records LOD sample step and mesh counts")),
        ("sampled_vertices", lambda: require_contains(f("terrain_cpp"), r"for\s*\([^)]*SampleZ.*SampleCellZs.*for\s*\([^)]*SampleX.*SampleCellXs.*GetHeight\s*\(\s*SampleX\s*,\s*SampleZ\s*\)", "vertices generated from sampled cell coordinates")),
        ("mesh_cell_indices", lambda: require_contains(f("terrain_cpp"), r"for\s*\([^)]*LocalZ\s*=.*Region\.MeshCellCountZ.*for\s*\([^)]*LocalX\s*=.*Region\.MeshCellCountX", "indices generated from mesh cell counts")),
        ("full_resolution_tile_loop_removed", lambda: require_not_contains(f("terrain_cpp"), r"const\s+Uint32\s+SampleCountX\s*=\s*Region\.CellCountX\s*\+\s*1u", "full-resolution sample count derived directly from source cell count")),
        ("terrain_lod_stats_api", lambda: require_contains(f("terrain_hpp"), r"GetMinLODSampleStep", "minimum LOD sample step API") or require_contains(f("terrain_hpp"), r"GetMaxLODSampleStep", "maximum LOD sample step API") or require_contains(f("terrain_hpp"), r"GetLastRenderedMeshCellCount", "rendered mesh cell count API")),
        ("terrain_lod_stats_members", lambda: require_contains(f("terrain_hpp"), r"m_MinLODSampleStep.*m_MaxLODSampleStep.*m_LastRenderedMeshCellCount", "terrain LOD sampling stats members")),
        ("renderer_lod_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainRenderedMeshCellCount.*TerrainMinLODSampleStep.*TerrainMaxLODSampleStep", "ForwardRenderer LOD sampling stats")),
        ("renderer_updates_lod_stats", lambda: require_contains(f("renderer_cpp"), r"TerrainRenderedMeshCellCount.*GetLastRenderedMeshCellCount.*TerrainMinLODSampleStep.*GetMinLODSampleStep.*TerrainMaxLODSampleStep.*GetMaxLODSampleStep", "ForwardRenderer updates LOD sampling stats")),
        ("editor_lod_stats", lambda: require_contains(f("editor_cpp"), r"Terrain rendered mesh cells.*LOD sample step", "ImGui LOD sampling stats")),
        ("status_record", lambda: require_contains(f("status"), r"LOD tile sampling", "status record for LOD tile sampling")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "LOD tile sampling validation record")),
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
        print("LOD tile sampling verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("LOD tile sampling verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
