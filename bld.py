if __name__ == "__main__":
    import sys, subprocess # noqa : E401
    sys.argv[0:1] = [sys.executable, "-m", "SCons", "-f", __file__]
    sys.exit(subprocess.run(sys.argv).returncode)

################################################################################

import scons_msvc_env as msvc_env

env = msvc_env.MsvcEnvironment(msvc_env.BuildCfg())
env.set_build_dir("src", "build")
env.Append(CPPPATH=["src"])
env.Append(CPPDEFINES=["_NO_CRT_STDIO_INLINE", "UNICODE", "_UNICODE"])
env.modify_flags("CCFLAGS", ["/W2"], ["/W4"])
env.use_pch()
env.adapt_for_dll()
env["SHLIBSUFFIX"] = ".wlx64"

src = ["bmp_from_svg.cpp", "svg_wlx.cpp"]
objs = env.PatchPchSym(source=env.Object(source=src))

libs = [
    "kernel32.lib",
    "user32.lib",
    "gdi32.lib",
    env.os_msvcrt_lib(),
    ]
dll, *_ = env.SharedLibrary("svg.wlx64", objs, LIBS=libs)

if env.sqaub_applicable():
    sdll = env.Squab(None, dll)
    env.Default(sdll)
env.install_relative_to_parent_dir("bin", dll)
