# Runtime changes relative to bloxtbc's upstream

Baseline: upstream `9e6e93ad7e7c38f9953b1f7bc65b1da666d78311`.
Current diagnostic version: `audio-sync-v5.16-garnier-diagnostics`.
This list covers the mod, not Hungarian translation or UI-archive editing.

## Audio matching and hook correctness

The Ubisoft DX9 callback can provide an event ID while the database uses the
referenced audio-resource ID. The original high-nibble substitution selected
the wrong entry for some Animus dialogue. The resolver prioritizes actual audio
IDs and follows only the verified direct-play resource-table layout, validating
bounds and copied records. Composite/random/conditional actions are left unresolved.

Audio and UI observer signatures are restricted to executable PE sections and
must match exactly once. Missing or ambiguous signatures disable the affected
observer. MinHook relocates the original audio CALL so its return value and side
effects reach the game. The observation stubs preserve registers, flags and
floating-point/SIMD state. The former pause stubs with decimal `108` instead of
`0x108` are no longer installed.

Permanent last-ID suppression was removed: a repeated playback can start the
same subtitle again. Audio notifications use a synchronized, bounded queue;
producer contention/capacity losses are counted. Missing entries, unresolved
resources and queue drops are visible in optional recording. William corridor
coverage additionally includes resource IDs established from captured events;
those data additions are not a universal ID-offset rule.

## Pause and resume

Subtitle segment and overall-duration deadlines share a monotonic playback
clock. The game menu's observed open/closed state decides whether it pauses,
covering Escape, mouse Resume and controller B/Back without assuming a keyboard
toggle. Nested menus and reopening during a pending resume are handled.

Scripted game-time freezes, such as an Animus tutorial explanation, are kept
separate from menu pauses. Dialogue continues during those freezes. In a real
pause menu the current subtitle is hidden, its remaining playback time is
retained, and it returns on resume. Every menu exit uses the same configurable
wall-time delay (`ResumeDelayMs=500` by default).

Mapped dialogue arriving at a pause boundary is buffered with bounded capacity.
Duplicates observed while paused are filtered; queued dialogue is replayed on
resume. The JSON wait/duration tags are not rewritten by pause handling.

## Layout and settings

- Font size follows the game-client display height, with a configurable base
  size, reference height and F1 multiplier. Desktop resolution is not used.
- Text wraps using the actual font metrics. Each explicit or wrapped line is
  horizontally centered and has its own background width.
- Default maximum width is 65% of the game display. Auto positioning grows the
  subtitle upward from a configurable bottom margin.
- Negative legacy padding is clamped to avoid clipping, and manual positions
  remain constrained to the display after resizing.
- A 1500 ms tail is added at runtime to each duration. New dialogue may still
  replace the active subtitle; original database timing tags remain intact.
- Save-file activity temporarily raises the subtitle. The default hold is
  3300 ms, including the requested extra 300 ms before lowering.
- The actual interaction-prompt text draw is observed to raise subtitles in
  Desmond's room. The old red-pixel heuristic and GPU readback were removed,
  avoiding false lifts during unrelated cutscenes.
- F1 retains a single size slider. Advanced placement/layout settings remain
  in the INI. Opening F1 does not clear active dialogue; saving settings preserves
  unrelated INI sections, including diagnostic preferences.

## Optional ambient priority

`subtitles_secondary.json` is an optional separate database. Ambient speech is
lower priority than mission/primary speech and cannot overwrite an active
primary subtitle. Primary dialogue can replace secondary dialogue. The file
format of the original main database remains compatible.

## Recording and review tools

- Recording is disabled by default and can be toggled live in F1. Explicit
  `StartOnLaunch=1` is supported; legacy `Enabled=1` settings do not silently
  enable recording on every run.
- Sessions append to local log/TSV files. Turning recording off flushes and
  closes both streams. Existing recordings are preserved.
- F2 adds a numbered marker with the active subtitle ID, text and elapsed
  playback time; a small on-screen confirmation links the log to gameplay video.
- Captures include raw/resolved audio IDs, mapping results, dispatch/display
  delay, subtitle and segment starts/ends, replacement/expiry reasons, visibility
  changes, suppression, pause buffering decisions, queue losses and skipped segments.
- Garnier has a separate F1 capture profile and separate `.garnier` log files.
  An unsuccessful profile switch preserves the prior recording session.
- Python review tools group repeated F2 presses, keep separate sessions/replays,
  report suspected early endings and retain unmapped/unresolved sound candidates.
  They do not infer speech from proximity alone or add guessed subtitle IDs.

## Verification and remaining limits

Eight native x86 tests cover lookup, queues, production detours, pause/menu
probing, layout, rendered-prompt observation, recording and event replay.
Seven Python tests cover F2 grouping and targeted Garnier report analysis.
The source build is checked independently of a user's translated game installation.

The user has reported the original tutorial mapping and recent menu pause/resume
behavior working. That is not a claim of complete game-wide subtitle coverage.
Garnier's remaining confession lines need a fresh audio/video-correlated capture.
The hook records notification times, not definitive audible sound start/end times.
