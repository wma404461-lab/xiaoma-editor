# Xiaoma Editor - PonyEdit

A C++ scene-production toolkit for Unreal Engine 5.6. The source includes import, naming, materials, pivot placement, surface scattering and editor panels. The plugin descriptor reports version 0.1.0. Available commands should not be mistaken for end-to-end acceptance of every workflow.

## Build and use

Place the repository in `Plugins/PonyEdit` inside your own C++ Unreal project. Regenerate project files, build the Editor target with Visual Studio, then enable PonyEdit. Other engine versions require API compatibility checks.

`Source/PonyEditEditor` contains the editor module. `Source/PonyEditRuntime` contains runtime settings. Both modules passed a clean Win64 Editor plugin build with Unreal Engine 5.6.0. Use an ASCII-only build path: the installed compiler failed on a non-ASCII intermediate path. The compiler also reported a non-preferred toolchain warning. Interactive editor workflows were not retested for this language-only source release.

Use a legitimately obtained Unreal Engine installation and C++ toolchain. Engine source, binaries, project assets and caches are not included. Project-authored plugin source is MIT licensed; Unreal Engine remains subject to its own license.
