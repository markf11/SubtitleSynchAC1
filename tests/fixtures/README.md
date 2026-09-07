`tutorial-v2.log` contains six consecutive audio notifications captured in the
Ubisoft DX9 tutorial with build `audio-sync-v2-resolved`. It retains numeric
metadata only. `subtitles.json` uses synthetic text, not game dialogue or the
user's translation. The replay must match only the resolved `0x20a802f9` ID;
surrounding unresolved/non-dialogue events must not start another subtitle.
This regression verifies event lookup, not audiovisual acceptance.
