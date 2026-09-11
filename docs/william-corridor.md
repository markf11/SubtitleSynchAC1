# William dialogue investigation

The reported behavior is: Altaïr's opening subtitle appears, William's early
answers are absent, and William's later answers appear. The available local log
contains no `0x10b5...` / `0x20b5...` events from this encounter, so it cannot
establish the missing audio-resource IDs or whether the current hook saw them.

The English and Hungarian databases contain William's late corridor answers at
`0x20b50008` (district, crime, conscription and discipline) and `0x20b50007`
(the fruits of Altaïr's labors), and Altaïr's replies at `0x20b50092`–`96`.
William's opening answers about his work, Conrad/Richard, the citizens and
stockpiling food are absent from those databases. This is a confirmed content
gap, not proof of a hook failure or a particular ID mapping. Do not fill the
numeric ID gaps by guessing.

To complete the repair, record this encounter with Diagnostics Enabled=1 and
correlate the spoken sentences with raw/resolved events and lookup misses.
Then add only verified audio-ID/text/duration pairs. If a missing sentence has
no event, investigate its playback route separately. The centered-layout build
does not claim to fix these missing corridor entries.

One separate Hungarian translation duplication is verified:
`0x20b50021` includes an extra form of address at its end, whereas the English
entry contains only the dismissal. The form of address belongs to the following
`0x20b50022`. `tools/fix_hu_william_duplicate.py` removes it from `21`, preserves
`22` and all timing tags, and only applies to the exact affected Hungarian text.
The English database is unchanged. The script is optional in redistribution
packages and never installs or replaces a full subtitle database.
