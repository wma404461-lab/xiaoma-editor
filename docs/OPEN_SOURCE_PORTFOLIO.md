# Xiaoma Open Source Portfolio

Maintainer: **Artist Ma** (professional name, also known as `artist m`) · [GitHub profile](https://github.com/wma404461-lab)

This portfolio covers all six public Xiaoma repositories verified on **October 24, 2026**. PonyEdit is the Unreal Engine component of a broader set of tools for scene artists and Windows creative workflows. Each project below has its own source repository, build instructions and license information.

## Projects at a glance

| Project | What it provides | Recorded engineering results |
| --- | --- | --- |
| [Xiaoma Editor / PonyEdit](https://github.com/wma404461-lab/xiaoma-editor) | Unreal Engine 5.6 asset import, naming, materials, pivots, surface scattering and editor panels | Clean Win64 Editor plugin build passed for both editor and runtime modules |
| [Xiaoma Asset Library](https://github.com/wma404461-lab/xiaoma-asset-library) | Local catalog for 3D models, textures, HDRIs and reference images | 11 core tests passed; independent renderer and C# thumbnail-helper build passed |
| [Xiaoma Player + Recorder](https://github.com/wma404461-lab/xiaoma-player) | Video, image and image-sequence viewing, document/model previews and selected-area recording | 8 core tests passed; frontend/helpers built; all 36 native C# source files compiled |
| [Xiaoma Translator / FlowSub](https://github.com/wma404461-lab/xiaoma-translator) | Live English-audio transcription and Chinese subtitles, with an English application interface | 114 automated tests passed; Python desktop overlay syntax check passed |
| [Xiaoma Archive](https://github.com/wma404461-lab/xiaoma-archive) | Native Windows archive management and background context-menu operations | x64 build and 49 archive self-tests passed |
| [Xiaoma Brightness](https://github.com/wma404461-lab/xiaoma-brightness) | Native Windows tray control for multiple displays, with hardware control and software dimming fallback | x86 build and 80,000 generated tray-logic checks passed |

The counts describe local checks of the published source snapshots. They are not user counts, adoption figures, independent certification or proof of every supported workflow. The repositories are newly public: each had 0 stars and 0 forks when checked on October 24, 2026; monthly downloads are not measured. This publication batch contains source, not downloadable binary releases.

## 1. Xiaoma Editor / PonyEdit

**Purpose:** Bring recurring scene-production operations into an Unreal Engine editor toolkit: importing assets, maintaining names, working with materials, positioning pivots and scattering objects on surfaces. It includes separate editor and runtime modules and editor panels. The descriptor reports version 0.1.0.

**Delivered work:** Published English source and build documentation. Both modules passed a clean Unreal Engine 5.6.0 Win64 Editor plugin build. The README records the tested toolchain limitations rather than presenting compatibility with other engine versions as established.

**Validation boundary:** The local toolchain needed an ASCII-only intermediate path and emitted a non-preferred compiler-version warning. Interactive editor workflows were not retested for the English source release.

**Inspect:** [Validated source snapshot](https://github.com/wma404461-lab/xiaoma-editor/tree/832c90873753e3d9ba175de32c89d2fc39444730) · [Build guide](https://github.com/wma404461-lab/xiaoma-editor/blob/832c90873753e3d9ba175de32c89d2fc39444730/README.md) · [Plugin descriptor](https://github.com/wma404461-lab/xiaoma-editor/blob/832c90873753e3d9ba175de32c89d2fc39444730/PonyEdit.uplugin)

## 2. Xiaoma Asset Library

**Purpose:** Organize local 3D models, textures, HDRIs and reference images in a catalog for creative asset browsing. Source version: 0.2.2. The public edition starts with an empty catalog and excludes personal asset indexes, source media, curated collections and private thumbnails.

**Delivered work:** Made the source edition independent of the neighboring player project, declared its dependencies, and fixed test-output directory creation for fresh checkouts. Eleven core tests and the renderer/C# thumbnail-helper build passed for the English edition.

**Validation boundary:** These checks do not establish support for every professional file format or external DCC version. Full interactive UI acceptance remains separate from the build and core tests.

**Inspect:** [Validated source snapshot](https://github.com/wma404461-lab/xiaoma-asset-library/tree/b9197571f19d6d25a6a42b5c48ffa742c091dd60) · [Build guide](https://github.com/wma404461-lab/xiaoma-asset-library/blob/b9197571f19d6d25a6a42b5c48ffa742c091dd60/README.md) · [Tests](https://github.com/wma404461-lab/xiaoma-asset-library/tree/b9197571f19d6d25a6a42b5c48ffa742c091dd60/tests)

## 3. Xiaoma Player + Recorder

**Purpose:** Review video, still images and image sequences, inspect document/model previews, and record a selected screen area. Source version: 0.6.23. The source includes a native WebView2 host and an Electron development entry point.

**Delivered work:** Published English UI text and documentation, corrected packaging-guide references, and restored the missing `UpdateResources.cs` input for the update helper. Eight core regressions, frontend/helper builds and compilation of all 36 native C# source files passed.

**Validation boundary:** The checks reused installed development dependencies. They do not certify a fresh-machine installer, every capture device, or all start/pause/stop/save paths in this public edition. A runtime hardware recording test was not performed during this publication batch.

**Inspect:** [Validated source snapshot](https://github.com/wma404461-lab/xiaoma-player/tree/afcce94253058cdd06b5fecc61d2cce4f345e9f4) · [Build guide](https://github.com/wma404461-lab/xiaoma-player/blob/afcce94253058cdd06b5fecc61d2cce4f345e9f4/README.md) · [Core tests](https://github.com/wma404461-lab/xiaoma-player/blob/afcce94253058cdd06b5fecc61d2cce4f345e9f4/tests/core.test.cjs)

## 4. Xiaoma Translator / FlowSub

**Purpose:** Provide live English-audio transcription and Chinese subtitles on Windows. Package version: 0.2.0. The application controls and documentation are English; the language-processing function intentionally still produces Chinese subtitles.

**Delivered work:** Published the application, translation pipeline and test sources. Preserved the original internal translation and speech-style prompts while localizing the interface, then passed 114 automated tests. The Python desktop overlay also passed a syntax check.

**Validation boundary:** This is not a fresh GPU-inference benchmark, an audio-hardware integration result or an accuracy rating for arbitrary speech. Model/runtime dependencies retain their separate licenses and setup requirements.

**Inspect:** [Validated source snapshot](https://github.com/wma404461-lab/xiaoma-translator/tree/d82bd99d2c10f52cd07f7b71b5d84f098ff31b7a) · [Setup and licensing](https://github.com/wma404461-lab/xiaoma-translator/blob/d82bd99d2c10f52cd07f7b71b5d84f098ff31b7a/README.md) · [Tests](https://github.com/wma404461-lab/xiaoma-translator/tree/d82bd99d2c10f52cd07f7b71b5d84f098ff31b7a/tests)

## 5. Xiaoma Archive

**Purpose:** Create ZIP, 7z and TAR archives and provide extraction, listing, validation, password and split-volume operations through a separately supplied 7-Zip engine. It has background Windows context-menu operations and an optional native management window.

**Delivered work:** Published the x64 source and build instructions, added explicit engine-directory support, and corrected removal of password-extraction menus in the uninstall script. Forty-nine self-tests passed, including Unicode paths/passwords, encrypted and split archives, cancellation and archive modification.

**Validation boundary:** The full local suite uses generated archive fixtures that are not included in the source edition. Native layout and every Windows/archive-format combination were not visually or operationally retested. The external 7-Zip engine has its own license.

**Inspect:** [Validated source snapshot](https://github.com/wma404461-lab/xiaoma-archive/tree/b372d109e9876ccbcf7fd57694ec09a03b47fb6d) · [Build and licensing guide](https://github.com/wma404461-lab/xiaoma-archive/blob/b372d109e9876ccbcf7fd57694ec09a03b47fb6d/README.md) · [Core regressions](https://github.com/wma404461-lab/xiaoma-archive/blob/b372d109e9876ccbcf7fd57694ec09a03b47fb6d/src/tests.cpp)

## 6. Xiaoma Brightness

**Purpose:** Adjust multiple Windows displays through a lightweight native tray slider or mouse-wheel interaction. It uses hardware brightness control where supported, software dimming as a fallback, and per-display remembered brightness. It can start silently for the current user without administrator privileges after its first normal launch.

**Delivered work:** Published the application, build script and tray regression sources. The English x86 build passed, together with 80,000 generated tray-logic checks. The suite exercises logic; the number is not a count of distinct manual UI scenarios.

**Validation boundary:** Native UI was not launched for this publication check. Monitor/driver support varies; ARM emulation and every physical display configuration have not been verified.

**Inspect:** [Validated source snapshot](https://github.com/wma404461-lab/xiaoma-brightness/tree/5a52b9d83457b411a67faf1169f2b422cf8a4b1b) · [Build guide](https://github.com/wma404461-lab/xiaoma-brightness/blob/5a52b9d83457b411a67faf1169f2b422cf8a4b1b/README.md) · [Tray test runner](https://github.com/wma404461-lab/xiaoma-brightness/blob/5a52b9d83457b411a67faf1169f2b422cf8a4b1b/scripts/test-tray-toggle.ps1)

## Maintenance and intended Codex use

The projects cover different parts of an artist's work: asset organization, scene preparation inside Unreal, media review and capture, live-language assistance, archive handling and display control. They are separate applications; this portfolio does not claim a shared integration layer.

Recorded maintenance includes source/dependency separation, English UI and documentation, reproducible build attempts, targeted regression fixes, license notices, and byte-level comparison of each reviewed publication tree with GitHub. Private assets, credentials, deployment data and dependency caches are excluded from the public source editions. Each repository's license and third-party notices govern its contents.

Planned Codex work includes review of C++ and JavaScript changes, issue reproduction, Unreal API compatibility checks, better automated coverage, build documentation and release preparation. Hardware and interactive UI acceptance will remain explicit tasks, with failures and limitations recorded. The portfolio does not claim a long public maintenance history, external contributors or adoption figures that have not been measured.
