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
        "tile_hpp": "LandscapeEditor/src/TerrainHeightmapTileSet.hpp",
        "tile_cpp": "LandscapeEditor/src/TerrainHeightmapTileSet.cpp",
        "patch_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "patch_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-heightmap-tile-metadata.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    patch_text = f("patch_hpp") + f("patch_cpp")
    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("cmake_tile_cpp", lambda: require_contains(f("cmake"), r"TerrainHeightmapTileSet\.cpp", "TerrainHeightmapTileSet source in CMake")),
        ("cmake_tile_hpp", lambda: require_contains(f("cmake"), r"TerrainHeightmapTileSet\.hpp", "TerrainHeightmapTileSet header in CMake")),
        ("tile_desc", lambda: require_contains(f("tile_hpp"), r"struct\s+TerrainHeightmapTileSetDesc.*TileCountX.*TileCountZ.*TileSampleCountPerAxis.*Extent", "heightmap tile set desc")),
        ("tile_type", lambda: require_contains(f("tile_hpp"), r"struct\s+TerrainHeightmapTile.*TileIndex.*TileX.*TileZ.*MinXZ.*MaxXZ.*SampleCountPerAxis", "heightmap tile metadata type")),
        ("tile_stats", lambda: require_contains(f("tile_hpp"), r"struct\s+TerrainHeightmapTileSetStats.*TileCountX.*TileCountZ.*TileCount.*TotalCellCountX.*TotalCellCountZ", "heightmap tile set stats")),
        ("tile_api", lambda: require_contains(f("tile_hpp"), r"class\s+TerrainHeightmapTileSet.*Build.*GetTiles.*FindTile.*GetStats.*GetLayoutName", "heightmap tile set API")),
        ("tile_build_grid", lambda: require_contains(f("tile_cpp"), r"for\s*\([^)]*TileZ.*for\s*\([^)]*TileX.*MinXZ.*MaxXZ.*CenterXZ", "tile grid bounds generation")),
        ("tile_sample_origins", lambda: require_contains(f("tile_cpp"), r"FirstSampleX.*FirstSampleZ.*TileCellCount", "tile sample origin metadata")),
        ("patch_desc_tiles", lambda: require_contains(f("patch_hpp"), r"TerrainPatchRendererDesc.*HeightmapTileCountX.*HeightmapTileCountZ", "TerrainPatchRenderer tile grid descriptor fields")),
        ("patch_owns_tileset", lambda: require_contains(f("patch_hpp"), r"TerrainHeightmapTileSet\s+m_HeightmapTileSet", "TerrainPatchRenderer owns heightmap tile set")),
        ("patch_builds_tileset", lambda: require_contains(f("patch_cpp"), r"m_HeightmapTileSet\.Build", "TerrainPatchRenderer builds heightmap tile set")),
        ("patch_tile_accessors", lambda: require_contains(f("patch_hpp"), r"GetHeightmapTileCountX.*GetHeightmapTileCountZ.*GetHeightmapPackageCellCountX.*GetHeightmapPackageCellCountZ", "TerrainPatchRenderer heightmap tile stats accessors")),
        ("renderer_tile_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainHeightmapTileCountX.*TerrainHeightmapTileCountZ.*TerrainHeightmapPackageCellCountX.*TerrainHeightmapPackageCellCountZ", "ForwardRenderer heightmap tile stats")),
        ("renderer_set_tile_grid", lambda: require_contains(f("renderer_hpp"), r"SetTerrainHeightmapTileGrid", "ForwardRenderer tile grid configuration API")),
        ("renderer_updates_tile_stats", lambda: require_contains(renderer_text, r"GetHeightmapTileCountX.*GetHeightmapTileCountZ.*GetHeightmapPackageCellCountX.*GetHeightmapPackageCellCountZ", "ForwardRenderer updates heightmap tile stats")),
        ("editor_cli_tiles", lambda: require_contains(editor_text, r"landscape_heightmap_tiles_x.*m_TerrainHeightmapTileCountX.*landscape_heightmap_tiles_z.*m_TerrainHeightmapTileCountZ", "LandscapeEditor heightmap tile grid CLI")),
        ("editor_forwards_tile_grid", lambda: require_contains(editor_text, r"SetTerrainHeightmapTileGrid\s*\([^;]*m_TerrainHeightmapTileCountX[^;]*m_TerrainHeightmapTileCountZ", "LandscapeEditor forwards tile grid config")),
        ("editor_ui_tiles", lambda: require_contains(f("editor_cpp"), r"Heightmap package:.*TerrainHeightmapLayoutName.*Heightmap package cells", "ImGui heightmap package stats")),
        ("status_record", lambda: require_contains(f("status"), r"heightmap tile metadata", "status record for heightmap tile metadata")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "heightmap tile metadata validation record")),
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
        print("heightmap tile metadata verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("heightmap tile metadata verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
