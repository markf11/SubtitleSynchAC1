# Resolution scaling and automatic wrapping

The overlay reads ImGui's game-client display dimensions every frame. The Win32
backend refreshes them with the game window's client rectangle, so fullscreen
resolution changes and window resizing update layout without opening F1. These
are overlay coordinates, not desktop resolution or an internal rendering scale.

Open **F1** for a live preview and settings, then **Save to .ini** to persist:

| Control | INI key in `[Subtitle]` | Default |
| --- | --- | --- |
| Scale with resolution | `AutoScale` | `1` |
| Base font size | `SubtitleFontSize` | `30` |
| Font scale (existing multiplier) | `SubtitleFontScale` | `1` |
| Reference height | `ReferenceHeight` | `1080` |
| Maximum width | `MaxWidthPercent` | `75` |

Effective size = base size × font scale × game display height / reference height.
When `AutoScale=0`, omit the last ratio. For the default settings, font sizes
are 20 px at 720p, 30 px at 1080p, 40 px at 1440p and 60 px at 2160p. Using height
keeps text from becoming larger merely because a monitor is ultrawide.
The F1 panel shows current display dimensions and effective font size.

Automatic wrapping is always enabled. It measures the shaped UTF-8 text using
the same font, effective size and width used to draw it. The width percentage
includes horizontal background padding. Each wrapped or explicit line is centered independently. Explicit newlines are preserved, and
long words can wrap as needed. The background grows with the text block.
Auto position centers the block horizontally and grows it upward from a bottom
margin of 100 px at 1080p (scaled with height). Manual positions are constrained
to the display after resizing. Excessively tall text blocks are not paginated.

Existing INI files remain compatible: missing keys use the defaults above.
Negative legacy padding is treated as zero to prevent glyph clipping. Newly
created settings use 8 px horizontal and 6 px vertical padding per side.
Settings changes do not modify subtitle text, timing or audio synchronization.

## Install and restore

Exit the game and back up `scripts/SubtitleSynchAC1.asi` and its INI. Replace
only the ASI with the new build; keep the installed loader, subtitle JSON, font
and INI. Add the keys above manually or use F1 and save. Restore the backed-up
ASI and INI with the game closed to revert. The patch ZIP contains no database.

The diagnostic startup tag is now `audio-sync-v4-centered`. Audio synchronization
was confirmed working by the user; the new layout has automated ImGui font
measurement tests at 720p, 1080p, 4K and portrait dimensions. In-game visual
acceptance of the new layout remains a separate check.
