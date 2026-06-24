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
        "height_hpp": "LandscapeEditor/src/TerrainHeightField.hpp",
        "height_cpp": "LandscapeEditor/src/TerrainHeightField.cpp",
        "patch_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "patch_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-24-non-square-heightfield-storage.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    height_text = f("height_hpp") + f("height_cpp")
    patch_text = f("patch_hpp") + f("patch_cpp")
    renderer_text = f("renderer_hpp") + f("renderer_cpp")

    checks = [
        ("height_desc_xy", lambda: require_contains(f("height_hpp"), r"CellCountX.*CellCountZ", "heightfield desc X/Z cell counts")),
        ("height_accessors_xy", lambda: require_contains(f("height_hpp"), r"GetCellCountX.*GetCellCountZ.*GetSampleCountX.*GetSampleCountZ.*GetCellSizeX.*GetCellSizeZ", "heightfield X/Z accessors")),
        ("height_storage_xy", lambda: require_contains(f("height_hpp"), r"m_SampleCountX.*m_SampleCountZ.*m_CellSizeX.*m_CellSizeZ", "heightfield rectangular storage members")),
        ("height_index_stride_x", lambda: require_contains(f("height_hpp"), r"GetIndex\s*\([^)]*\).*Z\s*\*\s*m_SampleCountX\s*\+\s*X", "heightfield row stride uses X sample count")),
        ("height_init_xy", lambda: require_contains(f("height_cpp"), r"m_Desc\.CellCountX.*m_Desc\.CellCountZ.*m_SampleCountX.*m_SampleCountZ.*m_CellSizeX.*m_CellSizeZ", "heightfield rectangular initialization")),
        ("height_tiled_combined_xy", lambda: require_contains(f("height_cpp"), r"CombinedCellCountX.*CombinedCellCountZ.*CombinedSampleCountX.*CombinedSampleCountZ", "tiled RAW R16 combined X/Z dimensions")),
        ("height_no_square_guard", lambda: require_not_contains(f("height_cpp"), r"TileCountX\s*!=\s*TileCountZ", "square-only tiled RAW R16 guard")),
        ("height_uv_xy", lambda: require_contains(f("height_cpp"), r"InvMaxCoordX.*InvMaxCoordZ", "independent X/Z UVs")),
        ("height_normals_xy", lambda: require_contains(f("height_cpp"), r"m_CellSizeX.*m_CellSizeZ.*HeightL.*HeightR.*HeightD.*HeightU", "normals use X/Z cell sizes")),
        ("height_clamp_xy", lambda: require_contains(f("height_cpp"), r"MaxCoordX.*m_SampleCountX.*MaxCoordZ.*m_SampleCountZ", "height clamping uses X/Z sample counts")),
        ("patch_accessors_xy", lambda: require_contains(f("patch_hpp"), r"GetCellCountX.*GetCellCountZ.*GetSampleCountX.*GetSampleCountZ", "TerrainPatchRenderer X/Z dimension accessors")),
        ("patch_draw_region_xy", lambda: require_contains(f("patch_cpp"), r"GetCellCountX.*GetCellCountZ.*InvSizeX.*InvSizeZ", "draw region maps X/Z dimensions independently")),
        ("patch_mesh_cellsize_xy", lambda: require_contains(f("patch_cpp"), r"GetCellSizeX.*GetCellSizeZ.*WorldX.*WorldZ", "packed mesh uses X/Z cell sizes")),
        ("renderer_stats_xy", lambda: require_contains(f("renderer_hpp"), r"TerrainCellCountX.*TerrainCellCountZ.*TerrainSampleCountX.*TerrainSampleCountZ", "ForwardRenderer X/Z terrain stats")),
        ("renderer_updates_xy", lambda: require_contains(renderer_text, r"GetCellCountX.*GetCellCountZ.*GetSampleCountX.*GetSampleCountZ", "ForwardRenderer updates X/Z terrain stats")),
        ("editor_no_non_square_reject", lambda: require_not_contains(f("editor_cpp"), r"non-square tiled RAW R16 heightmap package|TileCountX\s*!=\s*m_TerrainHeightmapTileCountZ", "editor non-square tiled package rejection")),
        ("editor_ui_xy", lambda: require_contains(f("editor_cpp"), r"Terrain cells: %u x %u.*TerrainCellCountX.*TerrainCellCountZ.*Terrain samples: %u x %u", "ImGui X/Z terrain dimension stats")),
        ("status_record", lambda: require_contains(f("status"), r"non-square heightfield|2 x 1.*RAW R16|3 x 2.*RAW R16", "status record for non-square heightfield storage")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "non-square heightfield validation record")),
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
        print("non-square heightfield verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("non-square heightfield verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
