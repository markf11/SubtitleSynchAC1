# Menu pause and punctuation update

The overlay uses a shared monotonic playback clock. When the game enters the
observed pause state, the clock stops; resuming subtracts all paused wall time.
Subtitle segment and total-duration deadlines stay on that clock. Repeated
pause notifications are idempotent. Audio events remain queued while paused so
they do not replace the frozen subtitle on the render thread.

The pause observer uses two unique executable-section signatures around the
original state writes. On the verified Ubisoft DX9 executable their MOV sites
are `0x00c774c5` (0) and `0x00c77b69` (1). MinHook relocates the native
instructions, including CMP at displacement `0x108`. Registers, flags and
floating-point state are preserved. No Esc-key toggling is used. If either
signature is missing or ambiguous, pause tracking is not installed. The initial
state is running until a transition is observed; use a fresh game start.

Diagnostics report `build=audio-sync-v5-pause`, `pause_hooks enabled`, and
`playback_pause paused=1/0`. Unit tests cover long/repeated pauses, retained
segment time and final expiry. A native x86 test executes both production
observers and verifies the original state writes and CMP flags. Six CTest
checks pass. Menu behavior in the actual game still needs acceptance testing.

For a manual check, open the pause menu during a long subtitle, wait longer than
its normal duration, then resume. The same segment should resume with only its
remaining active time. Repeat and also try submenus without returning to play.

`tools/normalize_hu_subtitle_punctuation.py` replaces remaining literal
semicolons with commas in a Hungarian subtitle JSON. It validates that no other
values or keys change and creates a timestamped backup before writing. Run with
`--apply` to write, or without it to inspect. The local installation contained
32 semicolons in 30 entries; their duration and wait tags are unchanged.

Exit the game before installing the ASI or editing its database. Restore the
backed-up ASI and JSON with the game closed to roll back. This update does not
replace the installed translation with the upstream English database.
