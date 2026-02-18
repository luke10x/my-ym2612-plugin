# CI/CD Setup

## GitHub Actions Workflows

### Windows Build

The `.github/workflows/build-windows.yml` workflow automatically builds the VST3 plugin for Windows.

**Triggers:**
- Push to tags matching `v*` (e.g., `v1.0.0`, `v32`)
- Manual workflow dispatch

**What it does:**
1. Sets up Windows build environment with MSVC
2. Installs CMake
3. Runs `build.bat` to build the VST3 plugin
4. Uploads the VST3 as a build artifact
5. On tagged releases, creates a GitHub release with the VST3 zip file

**To create a release:**
```bash
git tag v32
git push origin v32
```

This will trigger the build and automatically create a GitHub release with the compiled VST3.

**Manual build:**
You can also trigger builds manually from the Actions tab on GitHub.

## Local Windows Build

To build locally on Windows:

```cmd
build.bat
```

The VST3 will be in: `build_win\SquareWaveSynth_artefacts\Release\VST3\`

## Local macOS Build

To build locally on macOS:

```bash
./build.sh
```

The VST3 and AU will be in: `build/SquareWaveSynth_artefacts/Release/`
