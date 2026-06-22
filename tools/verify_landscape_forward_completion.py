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
    pattern = re.compile(r"\bvoid\s+([A-Za-z0-9_]+::Render)\s*\([^)]*\)\s*\{", re.MULTILINE)
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
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "queue_hpp": "LandscapeEditor/src/RenderQueue.hpp",
        "post_cpp": "LandscapeEditor/src/PostProcessRenderer.cpp",
        "status": "PROJECT_STATUS.md",
        "completion_plan": "docs/superpowers/plans/2026-06-22-forward-pipeline-completion.md",
        "hardening_plan": "docs/superpowers/plans/2026-06-22-forward-pipeline-hardening.md",
    }

    renderer_sources = [
        "LandscapeEditor/src/ForwardDebugPipeline.cpp",
        "LandscapeEditor/src/ForwardRenderer.cpp",
        "LandscapeEditor/src/PostProcessRenderer.cpp",
        "LandscapeEditor/src/SkyRenderer.cpp",
        "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "LandscapeEditor/src/TransparentRenderer.cpp",
    ]

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("cmake_all_passes", lambda: require_contains(f("cmake"), r"PostProcessRenderer\.(hpp|cpp).*SkyRenderer\.(hpp|cpp).*TransparentRenderer\.(hpp|cpp)", "all auxiliary pass renderers in CMake")),
        ("renderer_pass_members", lambda: require_contains(f("renderer_hpp"), r"ForwardDebugPipeline.*TerrainPatchRenderer.*ShadowRenderer.*SkyRenderer.*TransparentRenderer.*PostProcessRenderer", "ForwardRenderer owns all pass renderers")),
        ("renderer_pass_order", lambda: require_contains(f("renderer_cpp"), r"ShadowRenderer\.Render.*PrepareSceneTarget.*TerrainPatchRenderer\.Render.*SkyRenderer\.Render.*GetTransparentItems.*TransparentRenderer\.Render.*ForwardDebugPipeline\.Render.*PostProcessRenderer\.Render", "final forward pass order")),
        ("queue_transparent_sort", lambda: require_contains(f("queue_hpp"), r"AddTransparentQuad.*SortTransparentBackToFront.*GetTransparentItems", "transparent queue sorting")),
        ("postprocess_scene_target", lambda: require_contains(f("post_cpp"), r"BIND_RENDER_TARGET\s*\|\s*BIND_SHADER_RESOURCE", "postprocess scene-color target")),
        ("postprocess_scene_shader_resource", lambda: require_contains(f("post_cpp"), r"g_SceneColor", "postprocess scene-color shader resource")),
        ("opengl_fallback", lambda: require_contains(f("post_cpp"), r"IsGLDevice\(\).*CopyTexture", "OpenGL postprocess copy fallback")),
        ("status_bug_010", lambda: require_contains(f("status"), r"BUG-010.*OpenGL postprocess", "OpenGL postprocess issue documented")),
        ("completion_plan_exists", lambda: f("completion_plan")),
        ("hardening_plan_exists", lambda: f("hardening_plan")),
        ("no_pso_creation_in_render", lambda: require_no_pso_creation_in_render(renderer_sources)),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("forward completion verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("forward completion verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
