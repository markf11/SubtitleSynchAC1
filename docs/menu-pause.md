# Menu pause and punctuation update

The overlay uses a shared monotonic playback clock. When the game enters the
observed pause state, the clock stops; resuming subtracts all paused wall time.
Subtitle segment and total-duration deadlines stay on that clock. Repeated
pause notifications are idempotent. Mapped dialogue callbacks arriving during
the pause are buffered, near duplicates are filtered, and valid new dialogue is
replayed after resume. This prevents a line beginning near the pause boundary
from disappearing.

While paused, the subtitle is deliberately hidden so it does not cover the game
menu. Its active segment and remaining logical time stay intact. The first frame
after resume reveals the same text and continues its remaining duration.

The earlier executable signatures at `0x00c774c5` and `0x00c77b69` were proved
not to represent the ESC menu in the installed DX10 executable: the pause event
arrived after the subtitle had already expired, and closing the menu produced no
resume event. They are no longer installed. The render hook now observes the
rising edge of Escape while the game window has focus. The first press freezes
and hides the subtitle; holding the key cannot repeat the transition. The next
press marks the menu as closing, but the subtitle stays frozen and hidden until
that Escape is released and the configured `ResumeDelayMs` elapses. This keeps
the subtitle clock aligned with gameplay audio after the menu transition.

Diagnostics report `build=audio-sync-v5.9-tail-save-lift`,
`pause_tracking source=escape-edge+configured-resume-delay`, and
`playback_pause paused=1 source=escape-press` followed by
`paused=0 source=configured-resume-delay`. Unit tests cover deferred Escape release,
long/repeated pauses, retained
segment time and final expiry. A native x86 test executes both production
observers and verifies the original state writes and CMP flags. Six CTest
checks pass. The current input path prevents the subtitle from running during
the menu-closing keypress and preserves a new mapped dialogue event received
while the playback clock is frozen.

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
