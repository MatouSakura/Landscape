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
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-24-tiled-raw-r16-heightmap-package.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    height_text = f("height_hpp") + f("height_cpp")
    patch_text = f("patch_hpp") + f("patch_cpp")
    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("height_source_enum", lambda: require_contains(f("height_hpp"), r"RawR16Tiles", "RawR16Tiles heightfield source enum")),
        ("height_source_name", lambda: require_contains(f("height_cpp"), r"RawR16Tiles.*raw_r16_tiles", "raw_r16_tiles source name")),
        ("height_loaded_state", lambda: require_contains(f("height_hpp"), r"IsHeightmapLoaded.*RawR16.*RawR16Tiles", "heightmap loaded state includes tiled RAW R16")),
        ("height_tiled_api", lambda: require_contains(f("height_hpp"), r"LoadRawR16Tiles\s*\([^;]*TileCountX[^;]*TileCountZ[^;]*TileSampleCountPerAxis", "LoadRawR16Tiles API")),
        ("height_pattern_expansion", lambda: require_contains(f("height_cpp"), r"\{x\}.*\{z\}.*Replace|Replace.*\{x\}.*\{z\}|ExpandHeightmapTilePathPattern", "tile path token expansion")),
        ("height_tile_loops", lambda: require_contains(f("height_cpp"), r"for\s*\([^)]*TileZ.*for\s*\([^)]*TileX.*FirstSampleX.*FirstSampleZ", "tile merge loops with sample origins")),
        ("height_combined_grid", lambda: require_contains(f("height_cpp"), r"TileCellCount.*CombinedSampleCountPerAxis|CombinedCellCount.*TileCellCount", "combined heightfield dimensions")),
        ("height_reads_each_tile", lambda: require_contains(f("height_cpp"), r"ReadRawR16Samples|LoadRawR16Samples", "shared RAW R16 tile file reader")),
        ("patch_desc_pattern", lambda: require_contains(f("patch_hpp"), r"HeightmapRawR16TilesPattern", "TerrainPatchRenderer tiled RAW R16 pattern descriptor")),
        ("patch_loads_tiled_first", lambda: require_contains(patch_text, r"HeightmapRawR16TilesPattern.*LoadRawR16Tiles.*HeightmapRawR16Path", "TerrainPatchRenderer loads tiled RAW R16 before single RAW R16")),
        ("patch_tile_metadata_samples", lambda: require_contains(patch_text, r"TileSetDesc\.TileSampleCountPerAxis.*HeightmapSampleCountPerAxis", "tile metadata uses per-tile sample count for tiled package")),
        ("renderer_setter", lambda: require_contains(renderer_text, r"SetTerrainHeightmapRawR16Tiles", "ForwardRenderer tiled RAW R16 setter")),
        ("editor_member", lambda: require_contains(f("editor_hpp"), r"m_TerrainHeightmapRawR16TilesPattern", "LandscapeEditor tiled RAW R16 CLI member")),
        ("editor_cli", lambda: require_contains(editor_text, r"landscape_heightmap_raw_r16_tiles.*m_TerrainHeightmapRawR16TilesPattern", "LandscapeEditor parses tiled RAW R16 CLI")),
        ("editor_allows_non_square_tiles", lambda: require_not_contains(editor_text, r"non-square tiled RAW R16 heightmap package|TileCountX\s*!=\s*m_TerrainHeightmapTileCountZ", "LandscapeEditor non-square tiled RAW R16 rejection")),
        ("editor_forwards_pattern", lambda: require_contains(editor_text, r"SetTerrainHeightmapRawR16Tiles\s*\([^;]*m_TerrainHeightmapRawR16TilesPattern", "LandscapeEditor forwards tiled RAW R16 config")),
        ("status_record", lambda: require_contains(f("status"), r"tiled RAW R16 heightmap|RAW R16 tile", "status record for tiled RAW R16 heightmap package")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "tiled RAW R16 validation record")),
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
        print("tiled RAW R16 heightmap verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("tiled RAW R16 heightmap verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
