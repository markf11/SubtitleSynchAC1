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

The legacy pause/resume detours are not installed. The runtime samples the
game's AssassinMenuManager on each render frame. Its PrePauseEvent consumer
sets the byte at +0x5c to 1 on entry and 0 on exit. This menu state freezes the
logical clock; general PauseManager counts are recorded only for diagnostics.
This covers Escape, mouse Resume, controller menu selection, Start and Back:
input keys do not decide whether a menu is still open. A nested menu retains
its pause request; reopening cancels any pending resume.

Every exit from the final menu uses the same `ResumeDelayMs` wall-time gate
(default 500 ms), including controller and mouse. Previously those exits
resumed immediately while Escape alone used a delay. Normal database duration
and wait tags are unchanged. Escape edges are only a fallback when no supported
menu manager can be sampled. No keyboard heuristic overrides an observed menu
state. Null menu-manager instances mean there is no active menu, even if the
general PauseManager has not yet initialized.

Diagnostics report `build=audio-sync-v5.15-tutorial-pause`,
`pause_tracking source=menu-manager-state`, `menu_open`, general pause counts,
pending resume delay, and the
source of each playback transition. A runtime snapshot records the observer
addresses when recording is enabled after startup. Tests cover independent
menu transitions, nesting, reentry during the delay, zero-delay configuration,
long/repeated pauses and retained segment time.

## Scripted game-time freezes (v5.15)

v5.14 treated every active/committed PauseManager request as a menu pause.
AC1's FreezeTimeClip uses the same manager while dialogue can continue, so
Animus tutorial subtitles froze or disappeared and later audio was buffered.
v5.15 uses the menu state independently of those requests. Opening the menu
during an Animus explanation still freezes the subtitle. Closing it resumes
after the unchanged delay even if the tutorial's game-time request remains.
Database wait/duration tags and the +1.5-second tail are unchanged.

Both singleton and state-consumer signatures must match exactly once in an
executable section. The singleton's three pointer operands must agree; invalid
pointers or non-boolean state values are rejected. This is read-only sampling,
with no new injected game hook. In the inspected Ubisoft DX9 executable the
singleton is at VA 0xb44f40 and the state consumer at 0xb4511e; in the original
DX10 executable they are at 0x441a70 and 0x441dbe. Addresses are evidence only:
the implementation scans relocated signatures rather than using these values.
The DX9 FreezeTimeClip entry at 0x68e580 creates a general pause request through
0x4144f0 without sending PrePauseEvent. The menu component separately emits
entry/exit events at 0xc77b69 / 0xc774c5, consumed by AssassinMenuManager.

Regression tests run the actual probe against a mapped PE fixture and cover
scripted freeze, continued Animus segment timing, opening a menu during that
freeze, long pause, resuming with the tutorial request still active, exact
remaining duration, missing count source, invalid pointers/state, duplicate
signatures and inconsistent singleton operands. The eight x86 tests pass.
These are automated checks; the new build still needs tutorial and menu checks
in the game, including mouse Resume and controller B/Back.

For a manual check, open the pause menu during a long subtitle, wait longer than
its normal duration, then resume by Escape, by clicking Resume, and with a
controller. The same segment should resume with only its remaining active time.
Then replay the Animus tutorial: its explanations must advance during each
scripted time freeze, and a manually opened menu must still pause them.

`tools/normalize_hu_subtitle_punctuation.py` replaces remaining literal
semicolons with commas, four-or-more dots with three, and repeated exclamation
marks with one in a Hungarian subtitle JSON. It validates all keys and values
and creates a timestamped backup before writing. Run with `--apply` to write,
or without it to inspect. Duration and wait tags stay unchanged.

Exit the game before installing the ASI or editing its database. Restore the
backed-up ASI and JSON with the game closed to roll back. This update does not
replace the installed translation with the upstream English database.
