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
        raise AssertionError("PSO creation/cache lookup found in Render(): " + ", ".join(violations))


def main() -> int:
    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "heightfield_hpp": "LandscapeEditor/src/TerrainHeightField.hpp",
        "heightfield_cpp": "LandscapeEditor/src/TerrainHeightField.cpp",
        "terrain_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "terrain_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "sky_cpp": "LandscapeEditor/src/SkyRenderer.cpp",
        "post_cpp": "LandscapeEditor/src/PostProcessRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-heightfield-terrain-patch.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("file_heightfield_hpp", lambda: f("heightfield_hpp")),
        ("file_heightfield_cpp", lambda: f("heightfield_cpp")),
        ("cmake_heightfield_cpp", lambda: require_contains(f("cmake"), r"TerrainHeightField\.cpp", "TerrainHeightField source in CMake")),
        ("cmake_heightfield_hpp", lambda: require_contains(f("cmake"), r"TerrainHeightField\.hpp", "TerrainHeightField header in CMake")),
        ("heightfield_desc", lambda: require_contains(f("heightfield_hpp"), r"struct\s+TerrainHeightFieldDesc.*CellCount.*Extent.*HeightScale", "heightfield descriptor")),
        ("heightfield_stats", lambda: require_contains(f("heightfield_hpp"), r"struct\s+TerrainHeightSampleStats.*MinHeight.*MaxHeight.*AverageHeight", "heightfield stats")),
        ("heightfield_api", lambda: require_contains(f("heightfield_hpp"), r"GenerateProcedural.*GetCellCount.*GetSampleCountPerAxis.*GetHeight.*GetNormal.*GetUV.*GetStats", "heightfield public API")),
        ("heightfield_procedural_math", lambda: require_contains(f("heightfield_cpp"), r"std::sin.*std::cos.*std::sqrt", "deterministic procedural height formula")),
        ("heightfield_central_difference", lambda: require_contains(f("heightfield_cpp"), r"HeightL.*HeightR.*HeightD.*HeightU", "central-difference normal generation")),
        ("terrain_uses_heightfield", lambda: require_contains(f("terrain_hpp") + f("terrain_cpp"), r"TerrainHeightField", "TerrainPatchRenderer uses TerrainHeightField")),
        ("terrain_vertex_uv", lambda: require_contains(f("terrain_cpp"), r"float2\s+UV", "terrain vertex UV field")),
        ("terrain_input_layout_uv", lambda: require_contains(f("terrain_cpp"), r"LayoutElement\{2,\s*0,\s*2,\s*VT_FLOAT32", "UV input layout element")),
        ("terrain_shader_uv", lambda: require_contains(f("terrain_cpp"), r"TEXCOORD1.*UV", "terrain shader UV path")),
        ("terrain_no_flat_patch_height", lambda: require_not_contains(f("terrain_cpp"), r"PatchHeight", "flat PatchHeight constant")),
        ("terrain_heightfield_pso_keys", lambda: require_contains(f("terrain_cpp"), r"ForwardOpaque\.TerrainPatch\.Heightfield\.v1.*Shadow\.TerrainPatch\.Heightfield\.v1", "heightfield PSO cache keys")),
        ("terrain_glsl_language", lambda: require_contains(f("terrain_cpp"), r"SHADER_SOURCE_LANGUAGE_GLSL", "OpenGL GLSL terrain shader path")),
        ("terrain_glsl_shadow_sampler", lambda: require_contains(f("terrain_cpp"), r"uniform\s+sampler2D\s+g_ShadowMap0", "OpenGL GLSL terrain shadow sampler")),
        ("renderer_height_stats", lambda: require_contains(f("renderer_hpp"), r"TerrainCellCount.*TerrainSampleCountPerAxis.*TerrainMinHeight.*TerrainMaxHeight.*TerrainAverageHeight", "heightfield renderer stats")),
        ("renderer_stats_updates", lambda: require_contains(f("renderer_cpp"), r"GetCellCount.*GetSampleCountPerAxis.*GetMinHeight.*GetMaxHeight.*GetAverageHeight", "heightfield stats update")),
        ("renderer_gl_sky_before_opaque", lambda: require_contains(f("renderer_cpp"), r"View\.IsGL.*SkyRenderer\.Render.*GetOpaqueItems.*!View\.IsGL.*SkyRenderer\.Render", "OpenGL sky-before-opaque workaround")),
        ("editor_height_stats", lambda: require_contains(f("editor_cpp"), r"Terrain cells.*Height range.*Average height", "heightfield ImGui stats")),
        ("editor_bind_before_clear", lambda: require_contains(f("editor_cpp"), r"SetRenderTargets\s*\(.*ClearRenderTarget.*ClearDepthStencil", "render target binding before clear for OpenGL")),
        ("postprocess_bind_before_clear", lambda: require_contains(f("post_cpp"), r"PrepareSceneTarget.*SetRenderTargets\s*\(.*ClearRenderTarget", "scene target binding before postprocess clear for OpenGL")),
        ("sky_glsl_language", lambda: require_contains(f("sky_cpp"), r"SHADER_SOURCE_LANGUAGE_GLSL", "OpenGL GLSL sky shader path")),
        ("sky_glsl_far_depth", lambda: require_contains(f("sky_cpp"), r"gl_Position\s*=\s*vec4\s*\(\s*pos\s*,\s*1\.0\s*,\s*1\.0\s*\)", "OpenGL GLSL sky far-depth output")),
        ("status_heightfield_done", lambda: require_contains(f("status"), r"heightfield terrain patch", "heightfield status record")),
        ("status_quadtree_record", lambda: require_contains(f("status"), r"CPU quadtree node.*LOD selection|CPU quadtree LOD selection validation completed|Split terrain into CPU quadtree nodes", "quadtree status record")),
        ("plan_exists", lambda: f("plan")),
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
        print("heightfield verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("heightfield verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
