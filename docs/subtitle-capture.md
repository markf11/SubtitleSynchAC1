# Subtitle capture and timing review

For a short, separate capture of Garnier's death confession, use F1 →
**Record Garnier confession**. See [the focused recording guide](garnier-diagnostics.md).
This writes `SubtitleSynchAC1.garnier.log` and `SubtitleSynchAC1.garnier.capture.tsv`.
The normal recording switch and startup defaults are unchanged.

Recording is **off at every launch by default**. Open F1 and enable
**Record diagnostics and F2 markers** before replaying a scene. The switch
opens both `SubtitleSynchAC1.log` and `SubtitleSynchAC1.capture.tsv` beside the
ASI immediately, without restarting. Disable it to flush and close both files.
Old `Enabled=1` keys no longer cause automatic recording; they can be removed.
To explicitly record every launch, set:

```ini
[Diagnostics]
StartOnLaunch=1
```

Use `StartOnLaunch=0` for manual recording. The F1 switch applies to the current
run and does not change that startup preference. Existing recordings are
appended, preserved and identified by a new `session` row. Capture includes
IDs already present in `subtitles.json`, so known lines can be reviewed too.
Enable before a line starts if its full lifetime is needed. A mid-line start
records a visibility snapshot but cannot reconstruct an unrecorded audio dispatch.
The remaining rows contain:

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

To shortlist IDs near markers, run
`python tools/find_f2_candidates.py SubtitleSynchAC1.capture.tsv`. The marker
time alone does not identify which of several nearby sounds is speech; the
video or an extracted sound still needs to confirm the match. The requested
"Fast Forwarding Memory to a more recent one" line should only be placed in
`subtitles_secondary.json` after its ID and duration are confirmed this way.

AnvilToolkit can extract AC1's BAO resources from `.forge` archives. The
language-specific `DataPC_StreamedSoundseng.forge` extraction yields thousands
of numbered BAO data files but no readable line names. A data file by itself
may not decode: vgmstream needs the matching BAO header and any referenced
stream. The F2 log supplies the game's dispatched ID, not the BAO file name,
so a resource-ID-to-BAO match is another required step before playback.

New sessions append to the existing files. The uploaded test files contain
about 17 MB of detailed diagnostics and 13 MB of capture data across multiple
sessions, including unrelated testing. Volume depends on audio-event density;
record only the review runs if storage or noise is a concern.

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

Disable **Record diagnostics and F2 markers** in F1 after the review run.
F2 then stops making markers. Its visual confirmation never changes a subtitle.
The key is sampled continuously, even while recording is disabled, so an old
press made while recording was off cannot turn into a new marker when enabled.

For noisy multi-session files, use:

```text
python tools/find_f2_candidates.py SubtitleSynchAC1.capture.tsv --review-output review/F2-review
```

This writes a JSON/TSV index with session-relative times, grouped repeat presses,
active text and distinct nearby candidate IDs. `--session N` limits the report
to one capture session. Repeated presses during the same subtitle episode share
a group. Unmapped presses are grouped only within two seconds. Neither repeat
presses nor false presses add subtitles automatically: video/audio confirmation
is still required.

`subtitle_layout` diagnostic rows give the actual position, dimensions, line
count and separate save/interaction lift flags. This distinguishes a deliberate
prompt lift from the height change caused by wrapping.
