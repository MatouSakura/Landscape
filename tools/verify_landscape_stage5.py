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


def main() -> int:
    files = {
        "cmake": "LandscapeEditor/CMakeLists.txt",
        "frame_hpp": "LandscapeEditor/src/FrameResources.hpp",
        "frame_cpp": "LandscapeEditor/src/FrameResources.cpp",
        "renderer_hpp": "LandscapeEditor/src/ForwardRenderer.hpp",
        "renderer_cpp": "LandscapeEditor/src/ForwardRenderer.cpp",
        "terrain_hpp": "LandscapeEditor/src/TerrainPatchRenderer.hpp",
        "terrain_cpp": "LandscapeEditor/src/TerrainPatchRenderer.cpp",
        "shadow_hpp": "LandscapeEditor/src/ShadowRenderer.hpp",
        "shadow_cpp": "LandscapeEditor/src/ShadowRenderer.cpp",
    }

    def f(key: str) -> str:
        return read_text(files[key])

    checks = [
        ("file_shadow_hpp", lambda: f("shadow_hpp")),
        ("file_shadow_cpp", lambda: f("shadow_cpp")),
        ("cmake_shadow", lambda: require_contains(f("cmake"), r"ShadowRenderer\.(hpp|cpp)", "ShadowRenderer in CMake")),
        ("light_constants", lambda: require_contains(f("frame_hpp"), r"LightConstants.*ShadowViewProj.*CascadeSplits.*GetLightConstantsBuffer", "light constants API")),
        ("light_update", lambda: require_contains(f("frame_cpp"), r"UpdateLightConstants.*MapHelper<\s*LightConstants\s*>", "light constants update")),
        ("shadow_cascade_count", lambda: require_contains(f("shadow_hpp"), r"CascadeCount\s*=\s*4", "four cascade count")),
        ("shadow_depth_resources", lambda: require_contains(f("shadow_cpp"), r"BIND_SHADER_RESOURCE\s*\|\s*BIND_DEPTH_STENCIL|BIND_DEPTH_STENCIL\s*\|\s*BIND_SHADER_RESOURCE", "shadow depth SRV/DSV resources")),
        ("shadow_render", lambda: require_contains(f("shadow_cpp"), r"SetRenderTargets\(0,\s*nullptr.*ClearDepthStencil.*RenderShadow", "shadow pass render")),
        ("shadow_srv_access", lambda: require_contains(f("shadow_hpp"), r"GetCascadeSRV", "shadow cascade SRV accessor")),
        ("terrain_render_shadow", lambda: require_contains(f("terrain_hpp"), r"RenderShadow", "terrain shadow render API")),
        ("terrain_shadow_shader", lambda: require_contains(f("terrain_cpp"), r"ShadowViewProj.*g_ShadowMap0.*SampleLevel", "terrain shadow sampling shader")),
        ("renderer_owns_shadow", lambda: require_contains(f("renderer_hpp"), r"ShadowRenderer\s+m_ShadowRenderer", "ForwardRenderer shadow renderer member")),
        ("renderer_shadow_before_opaque", lambda: require_contains(f("renderer_cpp"), r"m_ShadowRenderer\.Render.*GetOpaqueItems", "shadow pass before opaque pass")),
        ("shadow_stats", lambda: require_contains(f("renderer_hpp"), r"ShadowCascadeCount.*ShadowMapSize", "shadow renderer stats")),
    ]

    failures = []
    for name, check in checks:
        try:
            check()
        except AssertionError as exc:
            failures.append(f"{name}: {exc}")

    if failures:
        print("stage5 verification failed:")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("stage5 verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
