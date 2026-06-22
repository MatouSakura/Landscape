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


def main() -> int:
    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "queue_hpp": "LandscapeEditor/src/RenderQueue.hpp",
        "sky_hpp": "LandscapeEditor/src/SkyRenderer.hpp",
        "sky_cpp": "LandscapeEditor/src/SkyRenderer.cpp",
        "transparent_hpp": "LandscapeEditor/src/TransparentRenderer.hpp",
        "transparent_cpp": "LandscapeEditor/src/TransparentRenderer.cpp",
        "post_hpp": "LandscapeEditor/src/PostProcessRenderer.hpp",
        "post_cpp": "LandscapeEditor/src/PostProcessRenderer.cpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("file_sky_hpp", lambda: f("sky_hpp")),
        ("file_sky_cpp", lambda: f("sky_cpp")),
        ("file_transparent_hpp", lambda: f("transparent_hpp")),
        ("file_transparent_cpp", lambda: f("transparent_cpp")),
        ("file_post_hpp", lambda: f("post_hpp")),
        ("file_post_cpp", lambda: f("post_cpp")),
        ("cmake_sky", lambda: require_contains(f("cmake"), r"SkyRenderer\.(hpp|cpp)", "SkyRenderer in CMake")),
        ("cmake_transparent", lambda: require_contains(f("cmake"), r"TransparentRenderer\.(hpp|cpp)", "TransparentRenderer in CMake")),
        ("cmake_post", lambda: require_contains(f("cmake"), r"PostProcessRenderer\.(hpp|cpp)", "PostProcessRenderer in CMake")),
        ("queue_transparent", lambda: require_contains(f("queue_hpp"), r"TransparentQuad.*AddTransparentQuad.*SortTransparentBackToFront.*GetTransparentItems", "transparent queue support")),
        ("sky_depth_fill", lambda: require_contains(f("sky_cpp"), r"DepthEnable\s*=\s*True.*DepthWriteEnable\s*=\s*False.*COMPARISON_FUNC_LESS_EQUAL", "sky far-depth fill state")),
        ("sky_pso_cache", lambda: require_contains(f("sky_cpp"), r"PSOCache\.GetOrCreate\(\"ForwardSky\.Procedural\.Fullscreen\"", "sky PSO cache key")),
        ("transparent_blend", lambda: require_contains(f("transparent_cpp"), r"BlendEnable\s*=\s*True.*BLEND_FACTOR_SRC_ALPHA.*BLEND_FACTOR_INV_SRC_ALPHA", "transparent alpha blend state")),
        ("transparent_pso_cache", lambda: require_contains(f("transparent_cpp"), r"PSOCache\.GetOrCreate\(\"ForwardTransparent\.TestQuad\.AlphaBlend\"", "transparent PSO cache key")),
        ("post_scene_target", lambda: require_contains(f("post_cpp"), r"BIND_RENDER_TARGET\s*\|\s*BIND_SHADER_RESOURCE", "postprocess scene color target")),
        ("post_scene_shader_resource", lambda: require_contains(f("post_cpp"), r"g_SceneColor", "postprocess scene color shader resource")),
        ("post_gl_shader_language", lambda: require_contains(f("post_cpp"), r"SHADER_SOURCE_LANGUAGE_GLSL", "OpenGL GLSL postprocess shader path")),
        ("post_gl_sampler", lambda: require_contains(f("post_cpp"), r"uniform\s+sampler2D\s+g_SceneColor", "OpenGL scene color sampler")),
        ("post_no_copy_fallback", lambda: require_not_contains(f("post_cpp"), r"CopyTexture", "OpenGL CopyTexture postprocess fallback")),
        ("post_pso_cache", lambda: require_contains(f("post_cpp"), r"PSOCache\.GetOrCreate\(\"PostProcess\.ToneMap\.Fullscreen\"", "postprocess PSO cache key")),
        ("renderer_members", lambda: require_contains(f("renderer_hpp"), r"SkyRenderer\s+m_SkyRenderer.*TransparentRenderer\s+m_TransparentRenderer.*PostProcessRenderer\s+m_PostProcessRenderer", "ForwardRenderer pass members")),
        ("renderer_order", lambda: require_contains(f("renderer_cpp"), r"ShadowRenderer\.Render.*PrepareSceneTarget.*TerrainPatchRenderer\.Render.*SkyRenderer\.Render.*GetTransparentItems.*TransparentRenderer\.Render.*ForwardDebugPipeline\.Render.*PostProcessRenderer\.Render", "forward pass order")),
        ("renderer_stats", lambda: require_contains(f("renderer_hpp"), r"SkyPassCount.*TransparentItemCount.*TransparentPassCount.*PostProcessPassCount", "renderer pass stats")),
        ("editor_stats", lambda: require_contains(f("editor_cpp"), r"Sky passes.*Transparent items.*Postprocess passes", "ImGui pass stats")),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("stage6 verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("stage6 verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
