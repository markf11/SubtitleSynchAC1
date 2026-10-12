# SubtitleSynchAC1 — reviewed runtime fork (v5.16)

This is a fork of [bloxtbc/SubtitleSynchAC1](https://github.com/bloxtbc/SubtitleSynchAC1),
originally developed by bloxtbc/Blox. The original MIT license and copyright
notice are retained. The overlay maps in-game audio notifications to local JSON
subtitles and renders them through ImGui.

This branch publishes the currently used runtime changes for review by the
original developer. The baseline is upstream commit
`9e6e93ad7e7c38f9953b1f7bc65b1da666d78311`. It does not publish the separate
Hungarian UI archives, extracted game files or private gameplay recordings.
The repository retains the upstream English database and earlier small,
optional subtitle patches; translation editing is separate from runtime behavior.

## Runtime changes

- Resolve verified event-to-audio resource references instead of guessing an ID prefix.
- Require unique hook signatures and preserve original calls, registers, flags and floating-point state.
- Allow the same audio ID to trigger subtitles again on a new playback.
- Pause subtitle time for the actual pause menu, including mouse and controller exits;
  keep Animus tutorial subtitles advancing during scripted game-time freezes.
- Resume retained dialogue after a configurable delay, without losing pause-boundary events.
- Scale fonts with the game window, wrap text automatically and center each line.
- Give each line its own measured background width and keep subtitles clear of save/interaction prompts.
- Extend subtitle visibility by a configurable 1.5-second tail, while allowing new dialogue to replace it.
- Keep F1 to one size slider, with advanced settings in the INI; opening F1 preserves active dialogue.
- Support optional secondary/ambient subtitles with mission dialogue taking priority.
- Provide opt-in recording, numbered F2 video markers, detailed segment/visibility timing,
  and a separate Garnier-confession capture and report tool.

See the [full runtime change list](docs/runtime-changes.md),
[source review and file-access notes](docs/source-review.md), and
[build instructions](docs/build-and-verify.md).

## Build and review

Clone with `git clone --recurse-submodules`. All four native dependencies are
pinned by their Git submodule commits. The mod and native tests must be built
as **32-bit Windows** binaries. Building does not modify a game installation or
copy the English database over an existing translation.

GitHub Actions builds and tests the committed source on Windows and publishes
an ASI-only artifact with its source commit, SHA-256 checksum and build manifest.
You can inspect the workflow and reproduce the build locally instead of using
an existing binary. See [build and verification](docs/build-and-verify.md).

## Install

With AC1 closed, back up and replace only `scripts/SubtitleSynchAC1.asi`.
Keep the existing ASI loader, `subtitles.json`, `subtitles.ttf`, optional
`subtitles_secondary.json`, and INI. F1 controls the size multiplier and recording;
F2 adds a numbered review marker while recording is enabled.

The [capture guide](docs/subtitle-capture.md), [layout guide](docs/responsive-subtitles.md),
[menu-pause notes](docs/menu-pause.md), and [Garnier recording guide](docs/garnier-diagnostics.md)
describe the current behavior. Recording is off at launch unless explicitly
enabled with `[Diagnostics] StartOnLaunch=1`.

## Limits

The live audio mapping was investigated on the Ubisoft DX9 build. DX10 has its
own render path and validated signature checks, but does not have equivalent
end-to-end gameplay acceptance. Unsupported/composite audio actions remain
unresolved rather than guessing a spoken line. Garnier's remaining confession
coverage is under investigation: the diagnostic mode is not a completed fix.
Automated tests and published source help review the mod; they do not constitute
an independent malware audit or proof that every existing binary matches the source.
