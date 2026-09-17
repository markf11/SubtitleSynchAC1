# Menu pause and punctuation update

The overlay uses a shared monotonic playback clock. When the game enters the
observed pause state, the clock stops; resuming subtracts all paused wall time.
Subtitle segment and total-duration deadlines stay on that clock. Repeated
pause notifications are idempotent. Audio callbacks are discarded while paused,
and both transition boundaries clear already queued callbacks. They therefore
cannot replace the frozen subtitle when gameplay resumes.

While paused, the subtitle is deliberately hidden so it does not cover the game
menu. Its active segment and remaining logical time stay intact. The first frame
after resume reveals the same text and continues its remaining duration.

The earlier executable signatures at `0x00c774c5` and `0x00c77b69` were proved
not to represent the ESC menu in the installed DX10 executable: the pause event
arrived after the subtitle had already expired, and closing the menu produced no
resume event. They are no longer installed. The render hook now observes the
rising edge of Escape while the game window has focus. The first press freezes
and hides the subtitle; holding the key cannot repeat the transition; the next
press resumes and reveals it.

Diagnostics report `build=audio-sync-v5.3-escape-pause`,
`pause_tracking source=escape-edge`, and
`playback_pause paused=1/0 queue_discarded=N source=escape`. Unit tests cover Escape
press/release edges, long/repeated pauses, retained
segment time and final expiry. A native x86 test executes both production
observers and verifies the original state writes and CMP flags. Six CTest
checks pass. The v5 log proved both game transitions fired, but also revealed
that queued callbacks replaced the frozen subtitle after resume. v5.1 fixes
that path. The v5.3 input path replaces the disproved executable observer; its
actual-game behavior still needs acceptance testing.

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
