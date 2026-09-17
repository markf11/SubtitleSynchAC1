# Resolution scaling and automatic wrapping

The overlay reads ImGui's game-client display dimensions every frame. The Win32
backend refreshes them with the game window's client rectangle, so fullscreen
resolution changes and window resizing update layout without opening F1. These
are overlay coordinates, not desktop resolution or an internal rendering scale.

Open **F1** for a live preview and the single `Subtitle scale` control, then
**Save to .ini** to persist it. Resolution scaling, wrapping, centering and
save-indicator positioning remain automatic. The advanced INI settings are:

| Setting | INI key in `[Subtitle]` | Default |
| --- | --- | --- |
| Scale with resolution | `AutoScale` | `1` |
| Base font size | `SubtitleFontSize` | `30` |
| Font scale (existing multiplier) | `SubtitleFontScale` | `1` |
| Reference height | `ReferenceHeight` | `1080` |
| Maximum width | `MaxWidthPercent` | `75` |
| Bottom margin at 1080p | `BottomMargin` | `100` |
| Tail extension | `TailExtensionMs` | `1500` |
| Lift during save activity | `SaveIndicatorLift` | `90` |
| Save indicator duration | `SaveIndicatorDurationMs` | `3300` |

Effective size = base size × font scale × game display height / reference height.
When `AutoScale=0`, omit the last ratio. For the default settings, font sizes
are 20 px at 720p, 30 px at 1080p, 40 px at 1440p and 60 px at 2160p. Using height
keeps text from becoming larger merely because a monitor is ultrawide.
The F1 panel shows current display dimensions and effective font size. Only
`SubtitleFontScale` is editable there; the other values stay available in the
INI for manual tuning.

Automatic wrapping is always enabled. It measures the shaped UTF-8 text using
the same font, effective size and width used to draw it. The width percentage
includes horizontal background padding. Each wrapped or explicit line is centered independently. Explicit newlines are preserved, and
long words can wrap as needed. The background grows with the text block.
Auto position centers the block horizontally and grows it upward from a bottom
margin of 100 px at 1080p (scaled with height). Manual positions are constrained
to the display after resizing. Excessively tall text blocks are not paginated.

Every database duration receives the configured tail extension at runtime. A new
audio event still replaces the current subtitle immediately. The JSON timing data
is not modified.

The mod watches AC1's own `%APPDATA%\\Ubisoft\\Assassin's Creed\\Saved Games`
directory. Save-file activity temporarily raises the subtitle by the configured
amount, then restores its normal position after `SaveIndicatorDurationMs`. This
avoids a version-sensitive hook into the game's `SaveGameClip`; both the lift and
the matching display time can be tuned directly in the INI.

Existing INI files remain compatible: missing keys use the defaults above.
Negative legacy padding is treated as zero to prevent glyph clipping. Newly
created settings use 8 px horizontal and 6 px vertical padding per side.
Settings changes do not modify subtitle text or audio synchronization. The tail
extension changes only the runtime end time and leaves JSON timing tags intact.

## Install and restore

Exit the game and back up `scripts/SubtitleSynchAC1.asi` and its INI. Replace
only the ASI with the new build; keep the installed loader, subtitle JSON, font
and INI. Add advanced keys manually; F1 saves the scale together with the
already loaded values. Restore the backed-up ASI and INI with the game closed
to revert. The patch ZIP contains no database.

The diagnostic startup tag is now `audio-sync-v5.10-simple-f1`. Audio synchronization
was confirmed working by the user; the new layout has automated ImGui font
measurement tests at 720p, 1080p, 4K and portrait dimensions. In-game visual
acceptance of the new layout remains a separate check.
