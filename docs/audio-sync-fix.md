# Audio synchronization repair

The Ubisoft DX9 audio callback can supply event IDs, while the subtitle database
uses audio resource IDs. Replacing the leading `1` with `2` was not reliable.
Live resource-table inspection found event `0x10a80305` referencing audio
`0x20a802f9`; the original implementation requested `0x20a80305` instead.

The resolver prioritizes original audio IDs and follows the verified direct-play
event reference. It checks manager bounds, root identity, action type and referenced
resource ID using copied memory. Composite/random/conditional actions remain
unresolved instead of selecting an arbitrary child or numeric offset.

## Hook contracts

- Scan executable PE sections and require exactly one signature match.
- Relocate and execute the original CALL through MinHook, preserving its return
  value and side effects.
- Preserve registers, flags and floating-point/SIMD state around observation.
- Do not install the former pause stubs that used decimal `108` instead of `0x108`.
- Remove the permanent last-ID filter so a new playback can reuse the same ID.
- Queue notifications with bounded capacity and account for dropped/contended pushes.
- Retain scheduled subtitle segment boundaries rather than accumulating frame delay.

The investigated Ubisoft DX9 EXE has SHA-256
`acb9e22d6023f96f9728205df563e2194768d157aa2c419624a15a528f74b944`.
The observed hook CALL was at RVA `0x3e8555`, with image base `0x400000`.
The implementation uses its checked signature, not a hard-coded CALL address.
The local EXE filename was misleadingly `AssassinsCreed_Dx10.exe`; executable
names alone do not establish the render build. No game EXE is committed here.

## Build, record and review

Use the [current build instructions](build-and-verify.md). Native tests execute
the actual x86 audio detour against synthetic memory and validate CALL effects,
return value, preserved state, ID matching and repeated capture.

Recording is off by default. F1 enables **Record diagnostics and F2 markers**
for the current run. For an explicit startup opt-in, set:

```ini
[Diagnostics]
StartOnLaunch=1
```

Old `Enabled=1` flags do not enable recording. Current diagnostics identify
`build=audio-sync-v5.16-garnier-diagnostics`. Resolution values are `0` for an
original audio ID, `1` for a verified direct reference, and `2` for unresolved.
The [capture guide](subtitle-capture.md) describes lifetime/segment/visibility
records and the [Garnier guide](garnier-diagnostics.md) describes focused review.
Notification times and database matches alone do not establish spoken-text
correctness or the actual audible start/end of a sound.

`replay_events` can review a saved diagnostic log without running the game:

```powershell
build/x86/replay_events.exe "SubtitleSynchAC1.log" "subtitles.json"
```

## Install and roll back

With the game closed, back up and replace only the ASI. Retain the installed
loader, translation JSON, font and INI. Restore the backed-up ASI with the game
closed to revert. Building does not copy assets into a game installation.
