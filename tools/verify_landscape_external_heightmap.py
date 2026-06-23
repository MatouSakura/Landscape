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
        "heightfield_hpp": "LandscapeEditor/src/TerrainHeightField.hpp",
        "heightfield_cpp": "LandscapeEditor/src/TerrainHeightField.cpp",
        "patch_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "patch_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-external-heightmap-raw-r16.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    heightfield_text = f("heightfield_hpp") + f("heightfield_cpp")
    renderer_text = f("renderer_hpp") + f("renderer_cpp")
    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("heightfield_source_enum", lambda: require_contains(f("heightfield_hpp"), r"enum\s+class\s+TerrainHeightFieldSource.*Procedural.*RawR16", "heightfield source enum")),
        ("heightfield_raw_r16_api", lambda: require_contains(f("heightfield_hpp"), r"LoadRawR16\s*\([^)]*std::string[^)]*SampleCountPerAxis", "TerrainHeightField RAW R16 load API")),
        ("heightfield_raw_r16_file_read", lambda: require_contains(f("heightfield_cpp"), r"std::ifstream.*std::ios::binary.*RawR16", "binary RAW R16 file loading")),
        ("heightfield_raw_r16_normalization", lambda: require_contains(f("heightfield_cpp"), r"65535\.0f.*HeightScale", "RAW R16 unsigned normalization through height scale")),
        ("heightfield_shared_finalize", lambda: require_contains(heightfield_text, r"RebuildNormals.*UpdateStats|UpdateStats.*RebuildNormals", "shared height stats and normal rebuild helpers")),
        ("patch_renderer_desc", lambda: require_contains(f("patch_hpp"), r"struct\s+TerrainPatchRendererDesc.*HeightmapRawR16Path.*HeightmapSampleCountPerAxis", "TerrainPatchRenderer heightmap descriptor")),
        ("patch_renderer_fallback", lambda: require_contains(f("patch_cpp"), r"HeightmapRawR16Path\.empty\(\).*GenerateProcedural|LoadRawR16.*GenerateProcedural", "procedural fallback when no heightmap is provided")),
        ("patch_renderer_source_accessors", lambda: require_contains(f("patch_hpp"), r"GetHeightSourceName.*IsHeightmapLoaded", "terrain height source accessors")),
        ("renderer_config_api", lambda: require_contains(f("renderer_hpp"), r"SetTerrainHeightmapRawR16", "ForwardRenderer heightmap configuration API")),
        ("renderer_source_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainHeightmapLoaded.*TerrainHeightSourceName", "ForwardRenderer heightmap source stats")),
        ("renderer_initializes_with_desc", lambda: require_contains(renderer_text, r"m_TerrainPatchRendererDesc.*Initialize\s*\([^;]*m_TerrainPatchRendererDesc", "ForwardRenderer passes terrain descriptor to TerrainPatchRenderer")),
        ("editor_cli_path", lambda: require_contains(editor_text, r"landscape_heightmap_raw_r16.*m_TerrainHeightmapRawR16Path", "LandscapeEditor raw R16 heightmap CLI path")),
        ("editor_cli_samples", lambda: require_contains(editor_text, r"landscape_heightmap_samples.*m_TerrainHeightmapSampleCountPerAxis", "LandscapeEditor heightmap sample count CLI")),
        ("editor_cli_height_scale", lambda: require_contains(editor_text, r"landscape_heightmap_height_scale.*m_TerrainHeightmapHeightScale", "LandscapeEditor heightmap height scale CLI")),
        ("editor_forwards_config", lambda: require_contains(editor_text, r"SetTerrainHeightmapRawR16\s*\([^;]*m_TerrainHeightmapRawR16Path", "LandscapeEditor forwards heightmap config")),
        ("editor_ui_source", lambda: require_contains(f("editor_cpp"), r"Height source:\s*%s.*TerrainHeightSourceName", "ImGui height source display")),
        ("status_record", lambda: require_contains(f("status"), r"external heightmap RAW R16", "status record for external heightmap RAW R16")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "external heightmap validation record")),
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
        print("external heightmap verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("external heightmap verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
