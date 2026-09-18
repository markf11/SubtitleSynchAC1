# Subtitle capture and timing review

The capture mode records every observed audio dispatch, including IDs already
present in `subtitles.json`. Enable it in `SubtitleSynchAC1.ini`:

```ini
[Capture]
Enabled=1
```

Restart the game, replay the scene, then exit normally. The mod writes
`SubtitleSynchAC1.capture.tsv` beside the ASI. Each session starts with a
`session` row. The remaining rows contain:

- `audio_dispatch`: the hook time, raw event ID, resolved resource ID and
  whether the ID maps to a primary, secondary or missing subtitle;
- `subtitle_start` and `subtitle_end`: the logical subtitle lifetime, database
  duration, effective duration including the configured tail, pause-excluded
  playback time and replacement/expiry reason;
- `subtitle_segment_start` and `subtitle_segment_end`: timing and text for each
  `<wait=...>` segment;
- `display_state`: exact visibility changes, including hiding and restoring the
  current line around the pause menu;
- `subtitle_suppressed`: a secondary subtitle that was correctly hidden while
  a primary subtitle was active;
- `review_marker_f2`: a numbered manual reference point made by pressing F2.

Press F2 at the audible end of a known spoken line. Its marker contains the
active subtitle ID and the pause-excluded elapsed time, which can be compared
with `db_duration_ms`. For a missing line, press F2 while it is spoken or at
its end; nearby `audio_dispatch` rows provide the candidate resource IDs.
F2 shows a small `F2 MARK #001` confirmation at the top of the screen for 1.2
seconds. The same marker number is written to the capture file so a gameplay
recording can be matched to the log. It does not hide or otherwise change the
subtitle.

Capture starts automatically on every game launch while `Enabled=1`. New
sessions are appended to the same file and identified by their own `session`
row. Based on measured event density, expect roughly 3-5 MB per gameplay hour,
or about 45-125 MB for a 15-25 hour run.

The runtime event contains an audio resource ID and timing metadata, not a
transcript. The spoken text must therefore be transcribed from the audio (by a
person or speech recognition) before it can be added to a subtitle database.
The hook's timestamp is the dispatch immediately before the game's original
sound call; it is labelled `audio_dispatch`, not an asserted sample-accurate
start time. The retail game does not ship the optional `SNDdbgV.DLL` API that
would expose sound lengths directly.

Put ambient additions such as citizen rescues and street speakers in
`subtitles_secondary.json`, using the same format as `subtitles.json`:

```json
{
  "0x20abcdef": "Translated text<duration=4.250>"
}
```

Entries in `subtitles.json` are primary. A primary line replaces a secondary
line immediately. A secondary line never replaces an active primary line.
When both files contain the same ID, the primary entry wins.

Disable capture after recording because a full session contains non-dialogue
audio events as well and can grow quickly:

```ini
[Capture]
Enabled=0
```
