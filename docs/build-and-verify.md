# Build and verify the source

The project is Windows-only and must compile as **x86 (32-bit)**. It requires
CMake 3.20+, C++17, Git and either Visual Studio 2022 with the C++ desktop workload
or LLVM-MinGW with its i686 Windows toolchain. Python 3 runs the optional review tests.

```powershell
git clone --recurse-submodules https://github.com/markf11/SubtitleSynchAC1.git
cd SubtitleSynchAC1
git submodule status --recursive
```

The exact dependency revisions are the Git submodule entries, not their moving
default branches. No game EXE or game installation is required to build or run
the automated tests.

## Visual Studio 2022

```powershell
cmake -S . -B build/msvc-x86 -G "Visual Studio 17 2022" -A Win32
cmake --build build/msvc-x86 --config Release --parallel
ctest --test-dir build/msvc-x86 -C Release --output-on-failure
python -m unittest discover -s tests -p test_f2_candidates.py -v
python -m unittest discover -s tests -p test_garnier_capture.py -v
```

The ASI is `build/msvc-x86/Release/SubtitleSynchAC1.asi`. Native tests execute
the real observation stubs in a synthetic x86 process, not a live game.
The MSVC build reads source text as UTF-8 so interaction-label literals do not
depend on the compiler machine's system code page.

## LLVM-MinGW / Ninja

```powershell
$env:LLVM_MINGW_ROOT = 'C:\tools\llvm-mingw'
$env:PATH = "$env:LLVM_MINGW_ROOT\bin;$env:PATH"
cmake -S . -B build/x86 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-x86.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/x86 --parallel
ctest --test-dir build/x86 --output-on-failure
python -m unittest discover -s tests -p test_f2_candidates.py -v
python -m unittest discover -s tests -p test_garnier_capture.py -v
```

The earlier installed v5.16 build used the official LLVM-MinGW 20260826 UCRT
distribution. Obtain tools from their respective official projects; no compiler,
prebuilt dependency DLL or ASI loader is committed here.

## CI artifact

The Windows workflow compiles this branch, runs eight native and seven Python
tests, then uploads the ASI and its source/checksum/build records. The artifact
contains no subtitle database or font, preventing an install from replacing an
existing translation. GitHub's workflow/run page associates the build with the
commit. Inspect the workflow before running it.

To create the same evidence locally after making a clean commit:

```powershell
python tools/write_build_manifest.py --binary build/msvc-x86/Release/SubtitleSynchAC1.asi --output build/runtime-artifact
```

The manifest generator copies the selected ASI into the artifact directory and
records its checksum, source commit, source hashes, dependencies and tool versions.
It does not install anything or contact a server. It rejects modified tracked
source/submodule checkouts. Raw checked-out text hashes can differ with CRLF/LF
conversion; the source commit is the canonical revision identifier.

Tests establish specific lookup, timing, layout and hook contracts. They do not
certify all game versions, every dialogue line or the absence of malware.
