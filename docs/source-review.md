# Source review and runtime access

This document is an inspection guide, not a malware certificate. The original
MIT license is preserved. Review the fork's diff against upstream commit
`9e6e93ad7e7c38f9953b1f7bc65b1da666d78311`, including its pinned dependencies.

## Entry point and runtime patching

`src/main.cpp` is the ASI/DLL entry point. It identifies the supported game
executable and creates an initialization thread inside the game process.
DX9/DX10 hooks intercept rendering and the game's window procedure to render
the overlay and its F1 panel.

`src/hooks/asm_hooks.cpp` observes the game's audio dispatch.
`src/hooks/press_prompt_hook.cpp` observes a bounded UTF-16 font-draw argument.
The stubs call the original game code through MinHook trampolines. Xbyak and
MinHook allocate executable memory and modify code inside the loaded game
process; this is how the ASI hook works, not an external injector.

`src/audio_resolver.cpp` and `src/pause_manager_probe.cpp` read bounded, copied
structures from the current process. Each `ReadProcessMemory` call uses
`GetCurrentProcess()`. There is no cross-process handle opened by this mod's
own runtime source. Unknown resource layouts/signatures are rejected.

## Files read and written

| Location | Runtime access | Purpose |
|---|---|---|
| Files beside the ASI: `SubtitleSynchAC1.ini` | Read; write on F1 Save | Overlay settings and explicit recording preference |
| Files beside the ASI: `subtitles.json`, optional `subtitles_secondary.json` | Read | Local subtitle lookup |
| Files beside the ASI: `subtitles.ttf` | Read | Overlay font |
| Files beside the ASI: `SubtitleSynchAC1.log`, `.capture.tsv`, and corresponding `.garnier` files | Append only while recording is enabled | Local debugging/review data |
| `%APPDATA%/Ubisoft/Assassin's Creed/Saved Games` | Directory change notifications; no save-content reads/writes | Temporarily raise subtitles while saving |

The mod does not extract, rewrite or redistribute game archives at runtime.
Building this branch does not install files or overwrite a translation database.
The unrelated EAX audio diagnostics plugin and game-archive translation tools
from the local workspace are not part of this publication's build.

## Input, network and recording scope

The mod's own source polls the specific F1, F2 and Escape keys. F2 and Escape
processing is limited to the focused game window. The ImGui panel consumes
window input for its controls and retains upstream text-editing/clipboard
behavior. It does not record a stream of arbitrary keystrokes.

The mod's own runtime code has no network client, remote update/download logic,
shell/process launcher, registry persistence or scheduled-task creation.
This statement is scoped to the published runtime source; review third-party
submodules too. Network access in the CI workflow is for cloning those pinned
public source dependencies and uploading the build artifact, not runtime behavior.

Opt-in logs include game executable path/PE metadata, process-local addresses,
audio IDs, timestamps and subtitle text. They contain no microphone/audio
recording. They stay local; the mod contains no log uploader. A user should
review logs before choosing to share them.

## Binary provenance and checksums

GitHub Actions builds the exact workflow commit, runs the tests and produces an
ASI-only artifact with `SOURCE-COMMIT.txt`, `SHA256SUMS.txt` and `BUILD-MANIFEST.json`.
The manifest records the source commit, checked-out source file hashes, dependency
commits and compiler/tool versions. The workflow's third-party actions are pinned
to full commit SHAs and its repository permission is read-only.

The separately recorded local v5.16 binary has SHA-256
`c9f88fa03efdf8e0fd4fafe1da5895316e18d797d066a6b07d546587e285e03c`.
It is recorded for identifying the earlier installed build, not offered as
cryptographic proof that it was built from this commit. Its build used
LLVM-MinGW 20260826 UCRT, x86, RelWithDebInfo. The CI build uses MSVC Release.
Different compiler versions, paths, timestamps and debug information can yield
different binary hashes; bit-for-bit reproducibility is not claimed.

Checksums identify bytes and do not establish benign behavior. Source review,
dependency review and building the ASI yourself are the intended verification
path. No independent security audit, antivirus certification or code signature
is claimed.
