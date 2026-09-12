# William corridor repair

The replayed scene supplied the missing events. Each resolves successfully to
an audio ID, but the original English and Hungarian databases lacked four spoken
entries. This repair adds those entries; it makes no change to the ASI hook.

| Audio ID | Content | English samples at 24 kHz | Duration |
| --- | --- | --- | --- |
| `0x20b5000d` | What do you know of my work? | 111283 | 4.637 s |
| `0x20b5000c` | Conrad, Richard, and who should own Acre | 459181 | 19.133 s |
| `0x20b5000b` | The city belongs to its people | 127666 | 5.319 s |
| `0x20b5000a` | Preparing for the new world and stockpiling food | 335688 | 13.987 s |

`0x20b50009` is the intervening cough, not another spoken reply. No dialogue
entry is added for it. Six reported sentence beginnings belong to four clips:
Conrad/Richard share one, and preparing/food share another.

## Evidence and timing

The numeric event fixture `tests/fixtures/william-corridor.log` comes from the
user's replay. All four formerly missing spoken IDs had `resolution=1` and
`hit=0`. The fixture includes the surrounding Altaïr replies and William's
existing district/discipline answer. After patching, nine of ten events match;
the sole non-match is the cough.

The BAO metadata and prefetch blocks were found in the local
`DataPC_Assassination_Talal.forge`, despite the scene being in Acre. The language
1 metadata/prefetch matches `DataPC_StreamedSoundseng.forge`. The stream IDs are
`51a361e2`, `5b59d53b`, `50a8ee47`, `56897d83`, respectively. They were decoded
locally using vgmstream. Existing entries `20b50007` and `08` independently
matched the decoded durations (20.468 and 18.554 seconds).

New duration tags follow sample counts. Segment boundaries use local speech
recognition word timestamps, checked against the user's reported sentence starts
and the local Hungarian transcript. The recognizer's spelling errors were not
copied into the English entries. The Conrad clip switches segments at 2.46,
7.32, 10.52 and 14.14 seconds; the food clip at 3.68 and 5.14 seconds. These are
initial audio-derived timings and still need in-game visual acceptance. Existing
subtitle entries and timing tags are preserved byte-for-byte within their values.
No audio, game binaries or extracted game resources are distributed.

## Applying the Hungarian patch

Exit the game, then run:

```
python tools/apply_william_subtitles.py "<game>/scripts/subtitles.json" --apply
```

The source checkout uses `assets/patches/william-corridor.hu.json`. A standalone
release places that JSON beside the script. Without `--apply`, the script only
checks the proposed patch. It creates a timestamped backup, preserves existing
entries, is idempotent, and refuses to overwrite conflicting translations.
The updated upstream English database includes the corresponding English entries;
do not install that full English database over a Hungarian translation.

The separate `fix_hu_william_duplicate.py` correction removes the duplicated
form of address from `0x20b50021` and keeps `0x20b50022` and their durations.

## Validation

The captured scene replays with 9/10 matches in both updated English and
Hungarian databases, with only the non-speech cough unmatched. All five CTest
checks pass. Merge validation checks all 2618 prior Hungarian entries stay
unchanged, four entries are added, repeated application is a no-op and conflicts
are rejected. In-game acceptance of the newly added lines is still pending.
