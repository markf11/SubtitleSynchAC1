# Audio synchronization repair (diagnostic build v2)

The Ubisoft DX9 audio callback supplies event IDs, while the subtitle database
uses audio resource IDs. Replacing the leading `1` with `2` is not a reliable
mapping. A live resource-table inspection found event `0x10a80305` referencing
audio `0x20a802f9`; the old implementation instead requested `0x20a80305`.
Events `0x10a800e3` and `0x10a800e4` happened to retain the same lower bits.

The new resolver preserves direct audio IDs and follows the verified direct-play
event reference. It checks manager bounds, root identity, action type and the
referenced resource ID using copied memory. Composite/random/conditional actions
remain unresolved: no arbitrary child or numeric offset is selected. This is a
deliberate coverage limit, visible as `resolution=2` in diagnostics.

## Hook and runtime changes

- Scan executable sections and require exactly one match. The signature includes
  the resource-manager lookup, event-ID load, arguments and result handling.
- Relocate and execute the original CALL through MinHook, preserving its return
  value and side effects. Preserve registers, flags and floating-point state
  around the observation callback.
- Remove redundant pause stubs. Native instructions retain their original
  `0x108` offsets; the old stubs incorrectly used decimal `108`.
- Remove permanent last-ID suppression. Repeated playback may restart subtitles.
- Use a synchronized bounded queue for multiple producers; count full/contended
  queue drops. A zero/unresolved ID cannot masquerade as an empty queue.
- Respect total subtitle duration even after delayed rendering, and retain
  scheduled segment boundaries instead of accumulating frame delay.
- Preserve extra INI sections when saving overlay preferences.
- Never copy translation assets into the output directory as a build step.

The current verified DX9 executable has SHA-256
`acb9e22d6023f96f9728205df563e2194768d157aa2c419624a15a528f74b944`.
The observed hook CALL is at RVA `0x3e8555`, targeting VA `0x7e4ca0` with
image base `0x400000`. Filenames can be misleading: the local DX9 executable
was named `AssassinsCreed_Dx10.exe`. Do not rename or replace executables as
part of installing this patch.

## Build and checks

Clone with `--recurse-submodules`. On Windows, install CMake, Ninja and
LLVM-MinGW, then run in PowerShell:

```powershell
$env:LLVM_MINGW_ROOT = 'C:\tools\llvm-mingw'
cmake -S . -B build/x86 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-x86.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/x86
ctest --test-dir build/x86 --output-on-failure
```

`subtitle_tests` checks the observed resource reference, exact lookup priority,
invalid/unresolved IDs, repeats, timing, queue accounting and pattern ambiguity.
`hook_tests` executes the production detour in a real x86 process and checks
the original CALL's side effect/result, preserved ESI and repeated resolved events.

With Python and `pefile`, run `python tools/verify_game.py "<game directory>"`
to verify signature uniqueness and compare subtitle ID/timing tags without
writing game files. The local Hungarian database has 2618 entries; its IDs and
timing tags match upstream. Automated checks do not establish which sentence
was actually spoken.

## Installation, diagnostics and rollback

Exit the game. Back up `scripts/SubtitleSynchAC1.asi` and its INI, plus your
translation JSON and font. Copy only `build/x86/SubtitleSynchAC1.asi` into
`scripts`. Keep the installed ASI loader, `subtitles.json`, `subtitles.ttf` and
existing INI settings. The patch package intentionally contains no subtitle DB.

Add this section to the existing INI to enable diagnostics:

```ini
[Diagnostics]
Enabled=1
```

Restart the game. `scripts/SubtitleSynchAC1.log` appends a startup record with
build tag `audio-sync-v2-resolved`, executable identity and hook matches.
Audio records include event time, producer thread, raw event ID, resolved audio
ID and resolution (`0` exact, `1` direct reference, `2` unresolved). Subsequent
records show database lookup, subtitle start/replacement, display changes and
queue drops. Set `Enabled=0` and restart to disable logging.

Replay a captured session offline with:

```powershell
build/x86/replay_events.exe "<SubtitleSynchAC1.log>" "<subtitles.json>"
```

The CSV reports database matches; it does not certify spoken-text correctness.
Keep logs and extracted game files out of the repository.

For rollback, exit the game and restore the backed-up ASI and INI. The
translation does not need restoration unless it was independently changed.

## Acceptance still required

Replay the Animus tutorial with English speech and Hungarian subtitles, record
any wrong/missing sentence and repeat the same playback. Correlate its event,
resolution, lookup and display records. For speech without a usable event,
investigate the actual playback route before extending the resolver or hook.
Test Altaïr's memory-corridor dialogue separately. Neither full tutorial coverage
nor memory-corridor coverage follows from the unit tests or resource snapshot.
