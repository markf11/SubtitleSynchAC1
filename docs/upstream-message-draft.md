# Message draft for the original developer

Hi bloxtbc,

I've been using SubtitleSynchAC1 on the Ubisoft PC version of Assassin's Creed 1
and made a fork to address missing/mismatched subtitles and add some runtime
features. The current source is at:

https://github.com/markf11/SubtitleSynchAC1

An installable ASI-only update, with source/build metadata and checksums, is here:

https://github.com/markf11/SubtitleSynchAC1/releases/tag/v5.16

The release tag points to the exact successful CI source commit. The public
source comparison against your upstream baseline is:

https://github.com/markf11/SubtitleSynchAC1/compare/9e6e93ad7e7c38f9953b1f7bc65b1da666d78311...v5.16

The main changes are verified event-to-audio resource mapping, safer hooks that
preserve the original game call, repeat playback handling, menu-aware pause/resume
for keyboard/mouse/controller, resolution-based sizing and centered wrapping with
per-line backgrounds, temporary save/interaction-prompt avoidance, an optional
subtitle tail, secondary/ambient priority, and opt-in timing diagnostics with F2
video markers. There is also a focused Garnier-confession recorder; that scene's
remaining missing coverage is still being investigated.

The Hungarian translation/UI work is separate from this runtime change list.
Your original license and attribution are preserved. I included a source review
guide covering the hooks and file access, build instructions, pinned submodule
revisions, automated tests, and a public Windows CI build with source/manifest/
checksum records. You can inspect the diff and build the ASI yourself rather
than trusting a supplied executable. I am not claiming an independent malware
audit or bit-for-bit reproducible binaries.

Most live validation has been on Ubisoft DX9; DX10 does not yet have the same
gameplay acceptance. If any of these changes would be useful upstream, I'd be
happy to split them into smaller patches for review.

Thanks,
markf11
