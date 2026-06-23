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
        "editor_hpp": "LandscapeEditor/src/LandscapeEditor.hpp",
        "editor_cpp": "LandscapeEditor/src/LandscapeEditor.cpp",
        "status": "PROJECT_STATUS.md",
        "plan": "docs/superpowers/plans/2026-06-23-scripted-camera-smoke.md",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    editor_text = f("editor_hpp") + f("editor_cpp")

    checks = [
        ("preset_enum", lambda: require_contains(f("editor_hpp"), r"enum\s+class\s+LandscapeCameraPreset.*Default.*MixedLOD.*OffFrustum", "LandscapeCameraPreset enum")),
        ("command_line_override", lambda: require_contains(f("editor_hpp"), r"ProcessCommandLine\s*\(\s*int\s+argc\s*,\s*const\s+char\*\s+const\*\s+argv\s*\)\s+override\s+final", "LandscapeEditor command-line override")),
        ("command_line_parser", lambda: require_contains(f("editor_cpp"), r"#include\s+\"CommandLineParser\.hpp\".*CommandLineParser\s+ArgsParser\s*\{\s*argc\s*,\s*argv\s*\}", "CommandLineParser usage")),
        ("preset_argument", lambda: require_contains(f("editor_cpp"), r"landscape_camera_preset.*default.*mixed_lod.*off_frustum", "landscape_camera_preset enum values")),
        ("invalid_argument_error", lambda: require_contains(f("editor_cpp"), r"LOG_ERROR_MESSAGE.*landscape_camera_preset.*default.*mixed_lod.*off_frustum", "invalid camera preset error")),
        ("preset_apply_function", lambda: require_contains(f("editor_hpp"), r"ApplyCameraPreset\s*\(\s*\)", "ApplyCameraPreset helper declaration")),
        ("preset_name_function", lambda: require_contains(f("editor_hpp"), r"GetCameraPresetName\s*\(\s*\)\s+const", "camera preset name helper declaration")),
        ("initialize_applies_preset", lambda: require_contains(f("editor_cpp"), r"SampleBase::Initialize\s*\(\s*InitInfo\s*\).*ApplyCameraPreset\s*\(\s*\).*UpdateRenderView\s*\(\s*\)", "preset applied before UpdateRenderView")),
        ("default_camera_values", lambda: require_contains(f("editor_cpp"), r"LandscapeCameraPreset::Default.*float3\s*\{\s*0\.0f\s*,\s*6\.0f\s*,\s*-14\.0f\s*\}.*SetRotation\s*\(\s*0\.0f\s*,\s*-0\.4f\s*\)", "default preset camera transform")),
        ("mixed_lod_camera_values", lambda: require_contains(f("editor_cpp"), r"LandscapeCameraPreset::MixedLOD.*SetPos\s*\(\s*float3\s*\{[^}]+\}\s*\).*SetRotation", "mixed LOD preset camera transform")),
        ("off_frustum_camera_values", lambda: require_contains(f("editor_cpp"), r"LandscapeCameraPreset::OffFrustum.*SetPos\s*\(\s*float3\s*\{[^}]+\}\s*\).*SetRotation", "off-frustum preset camera transform")),
        ("ui_preset_text", lambda: require_contains(f("editor_cpp"), r"Camera preset:\s*%s.*GetCameraPresetName\s*\(\s*\)", "ImGui camera preset display")),
        ("status_record", lambda: require_contains(f("status"), r"scripted camera", "status record for scripted camera smoke presets")),
        ("next_steps_updated", lambda: require_contains(f("status"), r"mixed-level LOD transition.*off-frustum culling.*Done|Done: Add camera/debug controls or scripted smoke positions", "next steps updated after scripted camera work")),
        ("plan_validation_record", lambda: require_contains(f("plan"), r"Validation Record.*Build:.*Static validation:.*Smoke:", "scripted camera validation record")),
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
        print("scripted camera smoke verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("scripted camera smoke verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
